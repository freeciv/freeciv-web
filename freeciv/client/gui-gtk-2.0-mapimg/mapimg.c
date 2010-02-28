/****************************************************************************
 Freeciv - Copyright (C) 2010 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#ifdef HAVE_WAND
  #include <wand/MagickWand.h>
#endif /* HAVE_WAND */

#include <gtk/gtk.h>

/* utility */
#include "astring.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "timing.h"

/* common */
#include "fc_types.h"
#include "game.h"
#include "map.h"
#include "terrain.h"
#include "tile.h"
#include "version.h"

/* client */
#include "colors.h"
#include "colors_common.h"
#include "tilespec.h"

#include "mapimg.h"

/* definitions for the different topologies */
#define TILE_SIZE 6
#define NUM_PIXEL TILE_SIZE * TILE_SIZE

BV_DEFINE(bv_pixel, NUM_PIXEL);

struct tile_shape {
  int x[NUM_PIXEL];
  int y[NUM_PIXEL];
};

typedef bv_pixel (*plot_func_t)(const int x, const int y);
typedef void (*base_coor_func_t)(int *base_x, int *base_y, int x, int y);

/* (isometric) rectangular topology */
static struct tile_shape tile_rect = {
  .x = {
    0, 1, 2, 3, 4, 5,
    0, 1, 2, 3, 4, 5,
    0, 1, 2, 3, 4, 5,
    0, 1, 2, 3, 4, 5,
    0, 1, 2, 3, 4, 5,
    0, 1, 2, 3, 4, 5
  },
  .y = {
    0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5
  }
};

static bv_pixel pixel_tile_rect(const int x, const int y);
static bv_pixel pixel_city_rect(const int x, const int y);
static bv_pixel pixel_unit_rect(const int x, const int y);
static bv_pixel pixel_fogofwar_rect(const int x, const int y);
static bv_pixel pixel_border_rect(const int x, const int y);
static void base_coor_rect(int *base_x, int *base_y, int x, int y);

/* hexa topology */
static struct tile_shape tile_hexa = {
  .x = {
          2, 3,
       1, 2, 3, 4,
    0, 1, 2, 3, 4, 5,
    0, 1, 2, 3, 4, 5,
    0, 1, 2, 3, 4, 5,
    0, 1, 2, 3, 4, 5,
       1, 2, 3, 4,
          2, 3,
  },
  .y = {
          0, 0,
       1, 1, 1, 1,
    2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5,
       6, 6, 6, 6,
          7, 7,
  }
};

static bv_pixel pixel_tile_hexa(const int x, const int y);
static bv_pixel pixel_city_hexa(const int x, const int y);
static bv_pixel pixel_unit_hexa(const int x, const int y);
static bv_pixel pixel_fogofwar_hexa(const int x, const int y);
static bv_pixel pixel_border_hexa(const int x, const int y);
static void base_coor_hexa(int *base_x, int *base_y, int x, int y);

/* isomeric hexa topology */
static struct tile_shape tile_isohexa = {
  .x = {
          2, 3, 4, 5,
       1, 2, 3, 4, 5, 6,
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7,
       1, 2, 3, 4, 5, 6,
          2, 3, 4, 5
  },
  .y = {
          0, 0, 0, 0,
       1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3,
       4, 4, 4, 4, 4, 4,
          5, 5, 5, 5
  }
};

static bv_pixel pixel_tile_isohexa(const int x, const int y);
static bv_pixel pixel_city_isohexa(const int x, const int y);
static bv_pixel pixel_unit_isohexa(const int x, const int y);
static bv_pixel pixel_fogofwar_isohexa(const int x, const int y);
static bv_pixel pixel_border_isohexa(const int x, const int y);
static void base_coor_isohexa(int *base_x, int *base_y, int x, int y);

/* image definition */
#define SPECENUM_NAME imageformat
#define SPECENUM_VALUE0 IMG_GIF
#define SPECENUM_VALUE1 IMG_PNG
#define SPECENUM_VALUE2 IMG_PPM
#define SPECENUM_VALUE3 IMG_JPG
#include "specenum_gen.h"

/* imageformat extensions */
static const char *image_extension[] = {
  "gif", "png", "ppm", "jpg", NULL
};

/* background color; used by MagickWand */
#define SPECENUM_NAME background
#define SPECENUM_VALUE0 BG_WHITE
#define SPECENUM_VALUE1 BG_BLACK
#define SPECENUM_VALUE2 BG_NONE
#include "specenum_gen.h"

/* background color string; used by MagickWand */
static const char *img_bg_color[] = {
  "white", "black", "none", NULL
};

/* foreground color string; used by MagickWand */
static const char *img_fg_color[] = {
  "black", "white", "black", NULL
};

/* player definitions */
#define SPECENUM_NAME show_player
#define SPECENUM_VALUE0 SHOW_NONE
#define SPECENUM_VALUE1 SHOW_PLRNAME
#define SPECENUM_VALUE2 SHOW_PLRID
#define SPECENUM_VALUE3 SHOW_PLRBV
#define SPECENUM_VALUE4 SHOW_EACH
#define SPECENUM_VALUE5 SHOW_ALL
#include "specenum_gen.h"

/* player definition strings */
static const char *str_showplr[] = {
  "none", "plrname", "plrid", "plrbv", "each", "all", NULL
};

/* map definition status */
#define SPECENUM_NAME mapimg_status
#define SPECENUM_VALUE0 MAPIMG_CHECK
#define SPECENUM_VALUE1 MAPIMG_OK
#define SPECENUM_VALUE2 MAPIMG_ERROR
#include "specenum_gen.h"

/* map definition status strings */
static const char *str_status[] = {
  "not checked", "OK", "error", NULL
};

#define MAX_LEN_STR 128
#define MAX_LEN_MAPARG MAX_LEN_STR

/* map definition */
struct mapdef {
  char maparg[MAX_LEN_MAPARG];
  char error[MAX_LEN_STR];
  enum mapimg_status status;
  enum imageformat imgformat;
  enum background bg;
  int zoom;
  int turns;
  bool terrain;
  bool cities;
  bool units;
  bool borders;
  bool known;
  bool fogofwar;
  bool area;
  struct {
    enum show_player show;
    union {
      char name[MAX_LEN_NAME];
      int id;
    } plr;
    bv_player plrbv;
  } player;
};

static struct mapdef *mapdef_default(void);
static void mapdef_destroy(struct mapdef *pmapdef);

#define SPECLIST_TAG mapdef
#define SPECLIST_TYPE struct mapdef
#include "speclist.h"

#define mapdef_list_iterate(mapdef_list, pmapdef) \
  TYPED_LIST_ITERATE(struct mapdef, mapdef_list, pmapdef)
#define mapdef_list_iterate_end \
  LIST_ITERATE_END

/* image definitions */
struct img {
  struct mapdef *def; /* map definition */
  int turn; /* save turn */
  char title[MAX_LEN_STR];

  /* topology definition */
  struct tile_shape *tileshape;
  plot_func_t pixel_tile;
  plot_func_t pixel_city;
  plot_func_t pixel_unit;
  plot_func_t pixel_fogofwar;
  plot_func_t pixel_border;
  base_coor_func_t base_coor;

  struct {
    int x;
    int y;
  } size; /* image size */
  struct color **map;
};

static struct img *img_new(struct mapdef *mapdef);
static void img_destroy(struct img *pimg);
static inline void img_set_pixel(struct img *pimg, const int index,
                                 struct color *pcolor);
static inline int img_index(const int x, const int y,
                            const struct img *pimg);
static void img_plot(struct img *pimg, const int x, const int y,
                     struct color *pcolor, const bv_pixel pixel);
static bool img_save(const struct img *pimg, const char *filename);
static void img_createmap(struct img *pimg);

/* mapimg data */
static struct {
  bool init;
  struct mapdef_list *mapdef;
} mapimg = { .init = FALSE };

static inline bool mapimg_initialised(void);
static bool mapimg_def2str(const char **str_def, struct mapdef *pmapdef);
static bool mapimg_checkplayers(struct mapdef *pmapdef);

#define MAX_NUM_MAPIMG 10

/* logging */
#define MAX_LEN_ERRORBUF 1024

static char error_buffer[MAX_LEN_ERRORBUF] = "\0";
static void mapimg_log(const char *file, const char *function, int line,
                       const char *format, ...)
                       fc__attribute((__format__(__printf__, 4, 5)));
#define MAPIMG_LOG(format, ...)                                             \
  mapimg_log(__FILE__, __FUNCTION__, __LINE__, format, ## __VA_ARGS__)
#define MAPIMG_RETURN_IF_FAIL(condition)                                    \
  if (!(condition)) {                                                       \
    MAPIMG_LOG("Assertion '%s' failed.", #condition);                       \
    return;                                                                 \
  }
#define MAPIMG_RETURN_VAL_IF_FAIL(condition, value)                         \
  if (!(condition)) {                                                       \
    MAPIMG_LOG("Assertion '%s' failed.", #condition);                       \
    return value;                                                           \
  }

static int generate_img_name(char *buf, int buflen,
                             const struct mapdef *pmapdef,
                             const char *basename);
static char *bvplayers_str(const struct mapdef *pmapdef);
static int bvplayers_count(const struct mapdef *pmapdef);

static bool game_was_started_todo(void);

/*
 * ==============================================
 * mapimg - external functions
 * ==============================================
 */

/**************************************************************************
  Initialisation of the map image subsystem.
**************************************************************************/
void mapimg_init(void)
{
  if (mapimg_initialised()) {
    return;
  }

#ifndef HAVE_WAND
  log_normal("no MagickWand available - falling back to ppm images");
#endif

  mapimg.mapdef = mapdef_list_new();

  mapimg.init = TRUE;
}

/**************************************************************************
  Reset the map image subsystem.
**************************************************************************/
void mapimg_reset(void)
{
  if (!mapimg_initialised()) {
    return;
  }

  if (mapdef_list_size(mapimg.mapdef) > 0) {
    mapdef_list_iterate(mapimg.mapdef, pmapdef) {
      mapdef_list_unlink(mapimg.mapdef, pmapdef);
      mapdef_destroy(pmapdef);
    } mapdef_list_iterate_end;
  }
}

/**************************************************************************
  Free all memory allocated by the map image subsystem.
**************************************************************************/
void mapimg_free(void)
{
  if (!mapimg_initialised()) {
    return;
  }

  mapimg_reset();
  mapdef_list_free(mapimg.mapdef);

  mapimg.init = FALSE;
}

/**************************************************************************
  Return the number of map image definitions.
**************************************************************************/
int mapimg_count(void)
{
  if (!mapimg_initialised()) {
    return 0;
  }

  return mapdef_list_size(mapimg.mapdef);
}

/**************************************************************************
  Return a help for the map definition
**************************************************************************/
const char *mapimg_help(void)
{
  enum imageformat format;
  enum background bg;
  enum show_player showplr;
  char tmp[128], tmp_bg[128], tmp_ext[128], tmp_showplr[128];
  static char help[2048];

  /* background color settings */
  tmp_bg[0] = '\0';
  for (bg = background_begin(); bg != background_end();
       bg = background_next(bg)) {
    if (bg != background_max()) {
      my_snprintf(tmp, sizeof(tmp), "'%s', ", img_bg_color[bg]);
      sz_strlcat(tmp_bg, tmp);
    } else {
      my_snprintf(tmp, sizeof(tmp), "'%s'", img_bg_color[bg]);
      sz_strlcat(tmp_bg, tmp);
    }
  }

  /* image extension settings */
  tmp_ext[0] = '\0';
  for (format = imageformat_begin(); format != imageformat_end();
       format = imageformat_next(format)) {
    if (format != imageformat_max()) {
      my_snprintf(tmp, sizeof(tmp), "'%s', ", image_extension[format]);
      sz_strlcat(tmp_ext, tmp);
    } else {
      my_snprintf(tmp, sizeof(tmp), "'%s'", image_extension[format]);
      sz_strlcat(tmp_ext, tmp);
    }
  }

  /* show player settings */
  tmp_showplr[0] = '\0';
  for (showplr = show_player_begin(); showplr != show_player_end();
       showplr = show_player_next(showplr)) {
    if (showplr != show_player_max()) {
      my_snprintf(tmp, sizeof(tmp), "'%s', ", str_showplr[showplr]);
      sz_strlcat(tmp_showplr, tmp);
    } else {
      my_snprintf(tmp, sizeof(tmp), "'%s'", str_showplr[showplr]);
      sz_strlcat(tmp_showplr, tmp);
    }
  }

  /* help text */
  my_snprintf(help, sizeof(help),
    _("Available map image options <mapdef>:\n"
      " * bg              background color (%s)\n"
      " * format          image format (%s)\n"
      " * show <value>    show players:\n"
      "                   (%s) \n"
      " * plrname <value> player name (if show=plrname)\n"
      " * plrid <value>   player id (if show=plrid)\n"
      " * plrbv <value>   player bitvector (if show=plrbv)\n"
      " * turns <value>   save each <value> turns (0 = no autosave)\n"
      " * zoom <value>    zoom factor (1 <= zoom <= 5)\n"
      " * map <mapdef>    definition of the map\n"
      "\n"
      "The map image definition <mapdef> string can contain: \n"
      " - 'a' show area within borders\n"
      " - 'b' show borders\n"
      " - 'c' show cities\n"
      " - 'f' show fogofwar\n"
      " - 'k' show only known terrain\n"
      " - 't' full terrain\n"
      " - 'u' show units\n"
      "\n"
      "'k' and 'f' are only available if the map is drawn for one player\n"
      "\n"
      "Examples:\n"
      " 'zoom=1:map=tcub:show=all:bg=none:format=gif'\n"
      " 'zoom=2:map=tcub:show=each:bg=black:format=png'\n"
      " 'zoom=1:map=tcub:show=player:player=Otto:bg=none:format=gif'\n"
      " 'zoom=3:map=cu:show=plrbv:plrbv=010011:bg=white:format=jpg'\n"
      " 'zoom=1:map=t:show=none:bg=white:format=jpg'"),
    tmp_bg, tmp_ext, tmp_showplr);

  return help;
}

/**************************************************************************
  Returns the last error.
**************************************************************************/
const char *mapimg_error(void)
{
  return error_buffer;
}

/**************************************************************************
  Define on map image.
**************************************************************************/
#define NUM_MAPARGS 10
#define NUM_MAPOPTS 2
bool mapimg_define(char *maparg, bool check)
{
  struct mapdef *pmapdef = NULL;
  char *mapargs[NUM_MAPARGS], *mapopts[NUM_MAPOPTS];
  char plrname[MAX_LEN_STR];
  int plrid;
  bv_player plrbv;
  int nmapargs, nmapopts, i;
  bool set_plrname = FALSE, set_plrid = FALSE, set_plrbv = FALSE;
  bool ret = TRUE;

  if (!mapimg_initialised()) {
    MAPIMG_LOG("Map images not initialised!");
    return FALSE;
  }

  if (maparg == NULL) {
    MAPIMG_LOG("No map definition");
    return FALSE;
  }

  if (strlen(maparg) > MAX_LEN_MAPARG) {
    /* to long map definition string */
    MAPIMG_LOG("Map definition string to long (max %d characters)",
               MAX_LEN_MAPARG);
    return FALSE;
  }

  if (mapimg_count() == MAX_NUM_MAPIMG) {
    MAPIMG_LOG("Maximal number of map definitions reached (%d)",
               MAX_NUM_MAPIMG);
    return FALSE;
  }

  pmapdef = mapdef_default();

  /* get map options */
  nmapargs = get_tokens(maparg, mapargs, NUM_MAPARGS, ":");

  for (i = 0; i < nmapargs; i++) {
    /* split map options into variable and value */
    nmapopts = get_tokens(mapargs[i], mapopts, NUM_MAPOPTS, "=");

    if (nmapopts == 2) {
      if (0 == strcmp(mapopts[0], "bg")) {
        /* background */
        enum background bg;
        bool found = FALSE;

        for (bg = background_begin(); bg != background_end();
             bg = background_next(bg)) {
          if (0 == strcmp(mapopts[1], img_bg_color[bg])) {
            pmapdef->bg = bg;
            found = TRUE;
            break;
          }
        }

        if (!found) {
          MAPIMG_LOG("invalid value for option %s: '%s'",
                     mapopts[0], mapopts[1]);
          ret = FALSE;
        }
      } else if (0 == strcmp(mapopts[0], "format")) {
        /* file format */
        enum imageformat format;
        bool found = FALSE;

        for (format = imageformat_begin(); format != imageformat_end();
             format = imageformat_next(format)) {
          if (0 == strcmp(mapopts[1], image_extension[format])) {
            pmapdef->imgformat = format;
            found = TRUE;
            break;
          }
        }

        if (!found) {
          MAPIMG_LOG("invalid value for option %s: '%s'",
                     mapopts[0], mapopts[1]);
          ret = FALSE;
        }
      } else if (0 == strcmp(mapopts[0], "map")) {
        /* map definition */
        int len = strlen(mapopts[1]), l;

        pmapdef->terrain = FALSE;
        pmapdef->cities = FALSE;
        pmapdef->units = FALSE;
        pmapdef->borders = FALSE;
        pmapdef->known = FALSE;
        pmapdef->fogofwar = FALSE;
        pmapdef->area = FALSE;

        for (l = 0; l < len; l++) {
          if (mapopts[1][l] == 't') {
            pmapdef->terrain = TRUE;
          } else if (mapopts[1][l] == 'c') {
            pmapdef->cities = TRUE;
          } else if (mapopts[1][l] == 'u') {
            pmapdef->units = TRUE;
          } else if (mapopts[1][l] == 'b') {
            pmapdef->borders = TRUE;
          } else if (mapopts[1][l] == 'k') {
            pmapdef->known = TRUE;
          } else if (mapopts[1][l] == 'f') {
            pmapdef->fogofwar = TRUE;
          } else if (mapopts[1][l] == 'a') {
            pmapdef->area = TRUE;
          } else {
            MAPIMG_LOG("Map definition string contains invalid "
                       "characters ('%c').", mapopts[1][l]);
            ret = FALSE;
          }
        }
      } else if (0 == strcmp(mapopts[0], str_showplr[SHOW_PLRNAME])) {
        /* player definition - player name; will be check by mapimg_isvalid()
         * which calls mapimg_checkplayers() */
        if (strlen(mapopts[1]) > sizeof(plrname)) {
          MAPIMG_LOG("Player name to long");
          ret = FALSE;
        } else {
          sz_strlcpy(plrname, mapopts[1]);
          set_plrname = TRUE;
        }
      } else if (0 == strcmp(mapopts[0], str_showplr[SHOW_PLRID])) {
        /* player definition - player id; will be check by mapimg_isvalid()
         * which calls mapimg_checkplayers() */
        if (sscanf(mapopts[1], "%d", &plrid) != 0) {
          if (plrid < 0 || plrid >= MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS) {
            MAPIMG_LOG("plrid must be in the intervall [0, %d)",
                       MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS);
            ret = FALSE;
          }
          set_plrid = TRUE;
        } else {
          MAPIMG_LOG("invalid value for option %s: '%s'",
                     mapopts[0], mapopts[1]);
          ret = FALSE;
        }
      } else if (0 == strcmp(mapopts[0], str_showplr[SHOW_PLRBV])) {
        /* player definition - bitvector */
        int i;

        if (strlen(mapopts[1]) < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS + 1) {
          BV_CLR_ALL(plrbv);
          for (i = 0; i < strlen(mapopts[1]); i++) {
            if (!strchr("01", mapopts[1][i])) {
              MAPIMG_LOG("Invalid character in bitvector: '%s'", mapopts[1]);
              ret = FALSE;
              break;
            } else if (mapopts[1][i] == '1') {
              BV_SET(plrbv, i);
            }
          }
        } else {
          MAPIMG_LOG("invalid value for option %s: '%s'",
                     mapopts[0], mapopts[1]);
          ret = FALSE;
        }
        set_plrbv = TRUE;
      } else if (0 == strcmp(mapopts[0], "show")) {
        /* player definition - basic definition */
        bool found = FALSE;
        enum show_player showplr;

        for (showplr = show_player_begin(); showplr != show_player_end();
             showplr = show_player_next(showplr)) {
          if (0 == strcmp(mapopts[1], str_showplr[showplr])) {
            pmapdef->player.show = showplr;
            found = TRUE;
            break;
          }
        }

        if (!found) {
          MAPIMG_LOG("invalid value for option %s: '%s'",
                     mapopts[0], mapopts[1]);
          ret = FALSE;
        }
      } else if (0 == strcmp(mapopts[0], "turns")) {
        /* save each <x> turns */
        int turns;

        if (sscanf(mapopts[1], "%d", &turns) != 0) {
          if (turns < 0 || turns > 99) {
            MAPIMG_LOG("turns should be between 0 and 99");
            ret = FALSE;
          } else {
            pmapdef->turns = turns;
          }
        } else {
          MAPIMG_LOG("invalid value for option %s: '%s'",
                     mapopts[0], mapopts[1]);
          ret = FALSE;
        }
      } else if (0 == strcmp(mapopts[0], "zoom")) {
        /* zoom factor */
        int zoom;

        if (sscanf(mapopts[1], "%d", &zoom) != 0) {
          if (zoom < 1 || zoom > 5) {
            MAPIMG_LOG("zoom factor should be between 1 and 5");
            ret = FALSE;
          } else {
            pmapdef->zoom = zoom;
          }
        } else {
        }
      } else {
        /* unknown option */
        MAPIMG_LOG("unknown map option: '%s'", mapargs[i]);
        ret = FALSE;
      }
    } else {
      MAPIMG_LOG("unknown map option: '%s'", mapargs[i]);
      ret = FALSE;
    }

    free_tokens(mapopts, nmapopts);

    if (!ret) {
      break;
    }
  }
  free_tokens(mapargs, nmapargs);

  /* sanity check */
  switch (pmapdef->player.show) {
  case SHOW_PLRNAME: /* display player given by name */
    if (!set_plrname) {
      MAPIMG_LOG("show=%s but no player name", str_showplr[SHOW_PLRNAME]);
      ret = FALSE;
    } else {
      sz_strlcpy(pmapdef->player.plr.name, plrname);
    }
    break;
  case SHOW_PLRID:   /* display player given by id */
    if (!set_plrid) {
      MAPIMG_LOG("show=%s but no player id", str_showplr[SHOW_PLRID]);
      ret = FALSE;
    } else {
      pmapdef->player.plr.id = plrid;
    }
    break;
  case SHOW_PLRBV:   /* display players given by bitvector */
    if (!set_plrbv) {
      MAPIMG_LOG("show=%s but no player bitvector", str_showplr[SHOW_PLRBV]);
      ret = FALSE;
    } else {
      BV_CLR_ALL(pmapdef->player.plrbv);
      for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
        if (BV_ISSET(plrbv, i)) {
          BV_SET(pmapdef->player.plrbv, i);
        }
      }
    }
    break;
  case SHOW_NONE:    /* no player one the map */
    BV_CLR_ALL(pmapdef->player.plrbv);
    break;
  case SHOW_ALL:     /* show all players in one map */
    BV_SET_ALL(pmapdef->player.plrbv);
    break;
  case SHOW_EACH:    /* one map for each player */
    BV_CLR_ALL(pmapdef->player.plrbv);
    break;
  }

  if ((pmapdef->cities == FALSE && pmapdef->units == FALSE
        && pmapdef->borders == FALSE && pmapdef->known == FALSE
        && pmapdef->fogofwar == FALSE && pmapdef->area == FALSE)) {
    pmapdef->player.show = SHOW_NONE;
    BV_CLR_ALL(pmapdef->player.plrbv);
  }

  if (ret && !check) {
    /* save map string */
    mystrlcpy(pmapdef->maparg, maparg, MAX_LEN_MAPARG);

    /* add map definiton */
    mapdef_list_append(mapimg.mapdef, pmapdef);
  } else {
    mapdef_destroy(pmapdef);
  }

  return ret;
}
#undef NUM_MAPARGS
#undef NUM_MAPOPTS

/**************************************************************************
  Check if a map image definition is valid. This function is a wrapper for
  mapimg_checkplayers().
**************************************************************************/
bool mapimg_isvalid(int id)
{
  struct mapdef *pmapdef = NULL;

  if (!mapimg_initialised()) {
    MAPIMG_LOG("Map images not initialised!");
    return FALSE;
  }

  if (id < 0 || id >= mapimg_count()) {
    MAPIMG_LOG("no map definition with id %d", id);
    return FALSE;
  }

  pmapdef = mapdef_list_get(mapimg.mapdef, id);

  if (!mapimg_checkplayers(pmapdef)) {
    return FALSE;
  }

  switch (pmapdef->status) {
  case MAPIMG_CHECK:
    MAPIMG_LOG("Map definition not checked (game not started).");
    return FALSE;
    break;
  case MAPIMG_ERROR:
    MAPIMG_LOG("Map definition deactivated: %s.", pmapdef->error);
    return FALSE;
    break;
  case MAPIMG_OK:
    /* nothing */
    break;
  }

  return TRUE;
}

/**************************************************************************
  Delete a map image definition.
**************************************************************************/
bool mapimg_delete(int id)
{
  struct mapdef *pmapdef = NULL;

  if (!mapimg_initialised()) {
    MAPIMG_LOG("Map images not initialised!");
    return FALSE;
  }

  if (id < 0 || id >= mapimg_count()) {
    MAPIMG_LOG("no map definition with id %d", id);
    return FALSE;
  }

  /* delete map definition */
  pmapdef = mapdef_list_get(mapimg.mapdef, id);
  mapdef_list_unlink(mapimg.mapdef, pmapdef);

  return TRUE;
}

/**************************************************************************
  Show a map image definition.
**************************************************************************/
bool mapimg_show(int id, const char **str_show, bool detail)
{
  static struct astring str = ASTRING_INIT;
  struct mapdef *pmapdef = NULL;

  if (!mapimg_initialised()) {
    MAPIMG_LOG("Map images not initialised!");
    return FALSE;
  }

  if (id < 0 || id >= mapimg_count()) {
    MAPIMG_LOG("no map definition with id %d", id);
    return FALSE;
  }

  /* set str to an empty string */
  astr_clear(&str);

  pmapdef = mapdef_list_get(mapimg.mapdef, id);

  if (detail) {
    astr_add_line(&str, "detailed information for map definition %d:", id);
    if (pmapdef->status == MAPIMG_ERROR) {
      astr_add_line(&str, "  - status:                   %s (%s)",
                    str_status[pmapdef->status], pmapdef->error);
    } else {
      astr_add_line(&str, "  - status:                   %s",
                    str_status[pmapdef->status]);
    }
    astr_add_line(&str, "  - imageformat:              %s",
                  image_extension[pmapdef->imgformat]);
    astr_add_line(&str, "  - background color:         %s",
                  img_bg_color[pmapdef->bg]);
    astr_add_line(&str, "  - text color:               %s",
                  img_fg_color[pmapdef->bg]);
    astr_add_line(&str, "  - zoom factor:              %d", pmapdef->zoom);
    astr_add_line(&str, "  - plot terrain:             %s",
                  pmapdef->terrain ? "full" : "basic");
    astr_add_line(&str, "  - plot cities:              %s",
                  pmapdef->cities ? "yes" : "no");
    astr_add_line(&str, "  - plot units:               %s",
                  pmapdef->units ? "yes" : "no");
    astr_add_line(&str, "  - plot borders:             %s",
                  pmapdef->borders ? "yes" : "no");
    astr_add_line(&str, "  - plot known tiles:         %s",
                  pmapdef->known ? "yes" : "no");
    astr_add_line(&str, "  - plot fogofwar:            %s",
                  pmapdef->fogofwar ? "yes" : "no");
    astr_add_line(&str, "  - plot area within borders: %s",
                  pmapdef->known ? "yes" : "no");
    astr_add_line(&str, "  - show players:             %s",
                  str_showplr[pmapdef->player.show]);
    switch (pmapdef->player.show) {
    case SHOW_NONE:
    case SHOW_EACH:
    case SHOW_ALL:
      /* nothing */
      break;
    case SHOW_PLRNAME:
      astr_add_line(&str, "  - player name:              %s",
                    pmapdef->player.plr.name);
      break;
    case SHOW_PLRID:
      astr_add_line(&str, "  - player id:                %d",
                    pmapdef->player.plr.id);
      break;
    case SHOW_PLRBV:
      astr_add_line(&str, "  - players:                  %s",
                    bvplayers_str(pmapdef));
      break;
    }

    astr_cut_lines(&str, 80);
  } else {
    const char *str_def;
    mapimg_def2str(&str_def, pmapdef);
    astr_add_line(&str, "'%s' (%s)", str_def, str_status[pmapdef->status]);
  }

  *str_show = str.str;

  return TRUE;
}

/**************************************************************************
  Create the requested map image. The filename is created as
  <basename>-<mapstr>.<mapext> where <mapstr> contains the map definition
  and <mapext> the selected image extension. If 'force' is FALSE, the image
  is only created if game.info.turn is a multiple of the map setting turns.
**************************************************************************/
bool mapimg_create(int id, const char *basename, bool force)
{
  struct mapdef *pmapdef = NULL;
  struct img *pimg;
  char filename[512]; /* MAX_LEN_FILENAME */
  bool ret = TRUE;
  struct timer *timer_cpu, *timer_user;

  if (!mapimg_initialised()) {
    MAPIMG_LOG("Map images not initialised!");
    return FALSE;
  }

  if (!game_was_started_todo()) {
    MAPIMG_LOG("no game loaded/started");
    return FALSE;
  }

  if (id < 0 || id >= mapimg_count()) {
    MAPIMG_LOG("no map definition with id %d", id);
    return FALSE;
  }

  pmapdef = mapdef_list_get(mapimg.mapdef, id);

  if (pmapdef->status != MAPIMG_OK) {
    MAPIMG_LOG("map definition not checked or error");
    return FALSE;
  }

  /* should an image be saved this turn? */
  if (!force && pmapdef->turns != 0
      && game.info.turn % pmapdef->turns != 0) {
    return TRUE;
  }

  /* check player selection */
  if (!mapimg_checkplayers(pmapdef)) {
    return FALSE;
  }

  timer_cpu = new_timer_start(TIMER_CPU, TIMER_ACTIVE);
  timer_user = new_timer_start(TIMER_USER, TIMER_ACTIVE);

  /* create map */
  switch (pmapdef->player.show) {
  case SHOW_PLRNAME: /* display player given by name */
  case SHOW_PLRID:   /* display player given by id */
    /* should never happen as player.show is set to SHOW_PLRBV for these
     * values in mapimg_checkplayers() */
    MAPIMG_RETURN_VAL_IF_FAIL((pmapdef->player.show == SHOW_PLRBV
                               || pmapdef->player.show == SHOW_EACH),
                              FALSE);
    break;
  case SHOW_NONE:    /* no player one the map */
  case SHOW_ALL:     /* show all players in one map */
  case SHOW_PLRBV:   /* display player(s) given by bitvector */
    generate_img_name(filename, sizeof(filename), pmapdef, basename);

    pimg = img_new(pmapdef);
    img_createmap(pimg);
    if (!img_save(pimg, filename)) {
      MAPIMG_LOG("Error saving file '%s'", filename);
      ret = FALSE;
    }
    img_destroy(pimg);
    break;
  case SHOW_EACH:    /* one map for each player */
    players_iterate(pplayer) {
      if (!pplayer->is_alive) {
        /* no map image for dead players */
        continue;
      }

      BV_CLR_ALL(pmapdef->player.plrbv);
      BV_SET(pmapdef->player.plrbv, player_index(pplayer));

      generate_img_name(filename, sizeof(filename), pmapdef, basename);
      pimg = img_new(pmapdef);
      img_createmap(pimg);
      if (!img_save(pimg, filename)) {
        MAPIMG_LOG("Error saving file '%s'", filename);
        ret = FALSE;
      }
      img_destroy(pimg);

      if (!ret) {
        break;
      }
    } players_iterate_end;
    break;
  }

  log_verbose("Image generation time: %g seconds (%g apparent)",
          read_timer_seconds(timer_cpu),
          read_timer_seconds(timer_user));

  free_timer(timer_cpu);
  free_timer(timer_user);

  return ret;
}

/**************************************************************************
  Create an image which shows all map colors (playercolor, terrain colors).
  A savegame has to be loaded for this function. THe filename will be
  <basename>-colortest.<default mapext>.
**************************************************************************/
bool mapimg_colortest(const char *basename)
{
  struct img *pimg;
  struct color *pcolor;
  struct mapdef *pmapdef;
  char buf[512]; /* MAX_LEN_FILENAME */
  bv_pixel pixel;
  int i, nat_x, nat_y, map_x, map_y;
  int max_playercolor = MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS;
  int max_terraincolor = terrain_count();
  bool ret = TRUE;

  if (!mapimg_initialised()) {
    MAPIMG_LOG("Map images not initialised!");
    return FALSE;
  }

  if (!game_was_started_todo()) {
    MAPIMG_LOG("no game loaded/started");
    return FALSE;
  }

  /* values for the test image */
  pmapdef = mapdef_default();
  pmapdef->bg = BG_WHITE;

  pimg = img_new(pmapdef);

  pixel = pimg->pixel_tile(1,1);

#define SIZE_X 10
#define SIZE_Y 5

  pcolor = get_color(tileset, COLOR_OVERVIEW_OCEAN);
  for (i = 0; i < MAX(max_playercolor, max_terraincolor); i++) {
    nat_x = 1 + i % SIZE_X;
    nat_y = 1 + (i / SIZE_X) * SIZE_Y;
    NATIVE_TO_MAP_POS(&map_x, &map_y, nat_x, nat_y);
    img_plot(pimg, map_x, map_y, pcolor, pixel);
  }

  for (i = 0; i < MAX(max_playercolor, max_terraincolor); i++) {
    if (i >= max_playercolor) {
      break;
    }

    nat_x = 1 + i % SIZE_X;
    nat_y = 2 + (i / SIZE_X) * SIZE_Y;
    pcolor = get_player_color(tileset, player_by_number(i));
    NATIVE_TO_MAP_POS(&map_x, &map_y, nat_x, nat_y);
    img_plot(pimg, map_x, map_y, pcolor, pixel);
  }

  pcolor = get_color(tileset, COLOR_OVERVIEW_LAND);
  for (i = 0; i < MAX(max_playercolor, max_terraincolor); i++) {
    nat_x = 1 + i % SIZE_X;
    nat_y = 3 + (i / SIZE_X) * SIZE_Y;
    NATIVE_TO_MAP_POS(&map_x, &map_y, nat_x, nat_y);
    img_plot(pimg, map_x, map_y, pcolor, pixel);
  }

  for (i = 0; i < MAX(max_playercolor, max_terraincolor); i++) {
    if (i >= max_terraincolor) {
      break;
    }

    nat_x = 1 + i % SIZE_X;
    nat_y = 4 + (i / SIZE_X) * SIZE_Y;
    pcolor = get_terrain_color(tileset, terrain_by_number(i));
    NATIVE_TO_MAP_POS(&map_x, &map_y, nat_x, nat_y);
    img_plot(pimg, map_x, map_y, pcolor, pixel);
  }

#undef SIZE_X
#undef SIZE_Y

  /* filename for color test */
  my_snprintf(buf, sizeof(buf), "%s-colortest", basename);

  if (!img_save(pimg, buf)) {
    MAPIMG_LOG("Error saving file '%s'", buf);
    ret = FALSE;
  } else {
    img_destroy(pimg);
  }

  return ret;
}

/**************************************************************************
  Return the map image definition 'id' as a mapdef string. This function is
  a wrapper for mapimg_def2str().
**************************************************************************/
bool mapimg_id2str(int id, const char **str_def)
{
  struct mapdef *pmapdef = NULL;

  if (!mapimg_initialised()) {
    MAPIMG_LOG("Map images not initialised!");
    return FALSE;
  }

  if (id < 0 || id >= mapimg_count()) {
    MAPIMG_LOG("no map definition with id %d", id);
    return FALSE;
  }

  pmapdef = mapdef_list_get(mapimg.mapdef, id);

  return mapimg_def2str(str_def, pmapdef);
}

/*
 * ==============================================
 * mapimg - internal functions
 * ==============================================
 */

/**************************************************************************
  Check if the map image subsustem is initialised.
**************************************************************************/
static inline bool mapimg_initialised(void)
{
  return mapimg.init;
}

/**************************************************************************
  Return a mapdef string for the map image definition given by 'pmapdef'.
**************************************************************************/
static bool mapimg_def2str(const char **str_def, struct mapdef *pmapdef)
{
  static struct astring str = ASTRING_INIT;
  char buf[7];
  int i;

  /* set str to the maparg used of the map definition */
  astr_clear(&str);
  astr_add_line(&str, "%s", pmapdef->maparg);
  *str_def = str.str;

  if (pmapdef->status != MAPIMG_OK) {
    MAPIMG_LOG("Map definition not checked or error.");
    return FALSE;
  }

  if (!game_was_started_todo()) {
    /* return uncheck definition string */
    return TRUE;
  }

  astr_clear(&str);

  i = 0;
  if (pmapdef->area) {
    buf[i++] = 'a';
  }
  if (pmapdef->borders) {
    buf[i++] = 'b';
  }
  if (pmapdef->cities) {
    buf[i++] = 'c';
  }
  if (pmapdef->fogofwar) {
    buf[i++] = 'f';
  }
  if (pmapdef->known) {
    buf[i++] = 'k';
  }
  if (pmapdef->terrain) {
    buf[i++] = 't';
  }
  if (pmapdef->units) {
    buf[i++] = 'u';
  }
  buf[i++] = '\0';

  astr_add(&str, "bg=%s:", img_bg_color[pmapdef->bg]);
  astr_add(&str, "format=%s:", image_extension[pmapdef->imgformat]);
  astr_add(&str, "turns=%d:", pmapdef->turns);
  astr_add(&str, "map=%s:", buf);
  switch (pmapdef->player.show) {
  case SHOW_NONE:
  case SHOW_EACH:
  case SHOW_ALL:
    /* nothing */
    astr_add(&str, "show=%s:", str_showplr[pmapdef->player.show]);
    break;
  case SHOW_PLRBV:
  case SHOW_PLRNAME:
  case SHOW_PLRID:
    astr_add(&str, "show=%s:", str_showplr[SHOW_PLRBV]);
    astr_add(&str, "plrbv=%s:", bvplayers_str(pmapdef));
    break;
  }
  astr_add(&str, "zoom=%d", pmapdef->zoom);

  *str_def = str.str;

  return TRUE;
}

/**************************************************************************
  Check the player selection (plrname and plrid).
**************************************************************************/
static bool mapimg_checkplayers(struct mapdef *pmapdef)
{
  struct player *pplayer;
  enum m_pre_result result;

  if (pmapdef->status != MAPIMG_CHECK) {
    return TRUE;
  }

  if (!game_was_started_todo()) {
    return TRUE;
  }

  pmapdef->status = MAPIMG_OK;

  /* game started - generate / check bitvector for players */
  switch (pmapdef->player.show) {
  case SHOW_NONE:
  case SHOW_PLRBV:
  case SHOW_EACH:
  case SHOW_ALL:
    /* nothing; see mapimg_define() */
    break;
  case SHOW_PLRNAME:
    /* display players as requested in the map definition */
    pmapdef->player.show = SHOW_PLRBV;
    BV_CLR_ALL(pmapdef->player.plrbv);
    pplayer = find_player_by_name_prefix(pmapdef->player.plr.name, &result);

    if (pplayer != NULL) {
      BV_SET(pmapdef->player.plrbv, player_index(pplayer));
    } else {
      pmapdef->status = MAPIMG_ERROR;
      my_snprintf(pmapdef->error, sizeof(pmapdef->error),
                  "unknown player name: '%s'", pmapdef->player.plr.name);
      MAPIMG_LOG("%s", pmapdef->error);
      return FALSE;
    }
    break;
  case SHOW_PLRID:
    /* display players as requested in the map definition */
    pmapdef->player.show = SHOW_PLRBV;
    BV_CLR_ALL(pmapdef->player.plrbv);
    pplayer = player_by_number(pmapdef->player.plr.id);

    if (pplayer != NULL) {
      BV_SET(pmapdef->player.plrbv, player_index(pplayer));
    } else {
      pmapdef->status = MAPIMG_ERROR;
      my_snprintf(pmapdef->error, sizeof(pmapdef->error),
                  "invalid player id: %d", pmapdef->player.plr.id);
      MAPIMG_LOG("%s", pmapdef->error);
      return FALSE;
    }
    break;
  }

  return TRUE;
}

/**************************************************************************
  Edit the error_buffer.
**************************************************************************/
static void mapimg_log(const char *file, const char *function, int line,
                       const char *format, ...)
{
  char message[MAX_LEN_ERRORBUF];
  va_list args;

  va_start(args, format);
  my_vsnprintf(message, sizeof(message), format, args);
  va_end(args);

#ifdef DEBUG
  my_snprintf(error_buffer, sizeof(error_buffer), "In %s() [%s:%d]: %s",
              function, file, line, message);
#else
  sz_strlcpy(error_buffer, message);
#endif
}

/*
 * ==============================================
 * mapdef
 * ==============================================
 */

/**************************************************************************
  Create a new map image definition with default values.
**************************************************************************/
static struct mapdef *mapdef_default(void)
{
  struct mapdef *pmapdef;

  pmapdef = fc_malloc(sizeof(*pmapdef));

  /* default values */
  pmapdef->maparg[0] = '\0';
  pmapdef->error[0] = '\0';
  pmapdef->status = MAPIMG_CHECK;
  pmapdef->imgformat = IMG_GIF;
  pmapdef->bg = BG_WHITE;
  pmapdef->zoom = 2;
  pmapdef->turns = 1;
  pmapdef->terrain = FALSE;
  pmapdef->cities = TRUE;
  pmapdef->units = TRUE;
  pmapdef->borders = TRUE;
  pmapdef->known = FALSE;
  pmapdef->fogofwar = FALSE;
  pmapdef->area = FALSE;
  pmapdef->player.show = SHOW_ALL;
  pmapdef->player.plr.name[0] = '\0';
  BV_CLR_ALL(pmapdef->player.plrbv);

  return pmapdef;
}

/**************************************************************************
  Destroy a map image definition.
**************************************************************************/
static void mapdef_destroy(struct mapdef *pmapdef)
{
  if (pmapdef == NULL) {
    return;
  }

  FC_FREE(pmapdef);
}

/*
 * ==============================================
 * mage functions
 * ==============================================
 */

/**************************************************************************
  Create a new image.
**************************************************************************/
static struct img *img_new(struct mapdef *mapdef)
{
  struct img *pimg;
  int i;

  pimg = fc_malloc(sizeof(*pimg));

  pimg->def = mapdef;
  pimg->turn = game.info.turn;
  my_snprintf(pimg->title, sizeof(pimg->title),
              _("Turn: %3d - Year: %s - %s"), game.info.turn,
              textyear(game.info.year),  game.server.save_name);

  pimg->size.x = 0; /* x size of the map image */
  pimg->size.y = 0; /* y size of the map image */

  if (topo_has_flag(TF_HEX)) {
    /* additional space for hex maps */
    pimg->size.x += TILE_SIZE / 2;
    pimg->size.y += TILE_SIZE / 2;

    if (topo_has_flag(TF_ISO)) {
      /* iso-hex */
      pimg->size.x += (map.xsize + map.ysize / 2) * TILE_SIZE;
      pimg->size.y += (map.xsize + map.ysize / 2) * TILE_SIZE;

      /* magic for isohexa: change size if wrapping in only one direction */
      if ((topo_has_flag(TF_WRAPX) && !topo_has_flag(TF_WRAPY))
          || (!topo_has_flag(TF_WRAPX) && topo_has_flag(TF_WRAPY))) {
        pimg->size.y += (map.xsize - map.ysize / 2) / 2 * TILE_SIZE;
      }

      pimg->tileshape = &tile_isohexa;

      pimg->pixel_tile = pixel_tile_isohexa;
      pimg->pixel_city = pixel_city_isohexa;
      pimg->pixel_unit = pixel_unit_isohexa;
      pimg->pixel_fogofwar = pixel_fogofwar_isohexa;
      pimg->pixel_border = pixel_border_isohexa;

      pimg->base_coor = base_coor_isohexa;
    } else {
      /* hex */
      pimg->size.x += map.xsize * TILE_SIZE;
      pimg->size.y += map.ysize * TILE_SIZE;

      pimg->tileshape = &tile_hexa;

      pimg->pixel_tile = pixel_tile_hexa;
      pimg->pixel_city = pixel_city_hexa;
      pimg->pixel_unit = pixel_unit_hexa;
      pimg->pixel_fogofwar = pixel_fogofwar_hexa;
      pimg->pixel_border = pixel_border_hexa;

      pimg->base_coor = base_coor_hexa;
    }
  } else {
    if (topo_has_flag(TF_ISO)) {
      /* isometric rectangular */
      pimg->size.x += (map.xsize + map.ysize / 2) * TILE_SIZE;
      pimg->size.y += (map.xsize + map.ysize / 2) * TILE_SIZE;
    } else {
      /* rectangular */
      pimg->size.x += map.xsize * TILE_SIZE;
      pimg->size.y += map.ysize * TILE_SIZE;
    }

    pimg->tileshape = &tile_rect;

    pimg->pixel_tile = pixel_tile_rect;
    pimg->pixel_city = pixel_city_rect;
    pimg->pixel_unit = pixel_unit_rect;
    pimg->pixel_fogofwar = pixel_fogofwar_rect;
    pimg->pixel_border = pixel_border_rect;

    pimg->base_coor = base_coor_rect;
  }

  pimg->map = fc_calloc(pimg->size.x * pimg->size.y, sizeof(*pimg->map));

  /* set initial map */
  for (i = 0; i < pimg->size.y * pimg->size.x; i++) {
    pimg->map[i] = NULL;
  }

  return pimg;
}

/**************************************************************************
  Destroy a image.
**************************************************************************/
static void img_destroy(struct img *pimg)
{
  if (pimg != NULL) {
    /* do not free pimg->def */
    FC_FREE(pimg->map);
    FC_FREE(pimg);
  }
}

/**************************************************************************
  Set the color of one pixel.
**************************************************************************/
static inline void img_set_pixel(struct img *pimg, const int index,
                                 struct color *pcolor)
{
  if (index < 0 || index > pimg->size.x*pimg->size.y) {
    log_error("invalid index: 0 <= %d < %d", index,
            pimg->size.x*pimg->size.y);
    return;
  }

  pimg->map[index] = pcolor;
}

/**************************************************************************
  Get the index for an (x,y) image coordinate.
**************************************************************************/
static inline int img_index(const int x, const int y,
                            const struct img *pimg)
{
  return (pimg->size.x)*y + x;
}

/**************************************************************************
  Plot one tile. Only the pixel of the tile set within 'pixel' are ploted.
**************************************************************************/
static void img_plot(struct img *pimg, const int x, const int y,
                     struct color *pcolor, const bv_pixel pixel)
{
  int base_x, base_y, i, index;

  if (!BV_ISSET_ANY(pixel)) {
    return;
  }

  pimg->base_coor(&base_x, &base_y, x, y);

  for (i = 0; i < NUM_PIXEL; i++) {
    if (BV_ISSET(pixel, i)) {
      index = img_index(base_x + pimg->tileshape->x[i],
                        base_y + pimg->tileshape->y[i],
                        pimg);
      img_set_pixel(pimg, index, pcolor);
    }
  }
}

/**************************************************************************
  Save an image. If freeciv is compiled with ImageMagick a lot of file
  formats are possible. Else fall back to a ppm image.

  Image structure:

  [             0]
                    border
  [+BORDER_HEIGHT]
                    title
  [+  TEXT_HEIGHT]
                    space (only if count(displayed players) > 0)
  [+SPACER_HEIGHT]
                    player line (only if count(displayed players) > 0)
  [+  LINE_HEIGHT]
                    space
  [+SPACER_HEIGHT]
                    map
  [+   MAP_HEIGHT]
                    border
  [+BORDER_HEIGHT]

**************************************************************************/
#ifdef HAVE_WAND
#define BORDER_HEIGHT 5
#define BORDER_WIDTH BORDER_HEIGHT
#define SPACER_HEIGHT 5
#define LINE_HEIGHT 5
#define TEXT_HEIGHT 12
#define SET_COLOR(str, pcolor)                                               \
  my_snprintf(str, sizeof(str), "rgb(%d,%d,%d)",                             \
              pcolor->color.red >> 8, pcolor->color.green >> 8,              \
              pcolor->color.blue >> 8);
static bool img_save(const struct img *pimg, const char *filename)
{
  struct color *pcolor = NULL;
  struct player *pplr_now = NULL, *pplr_only = NULL;
  bool ret = TRUE;
  char imagefile[512]; /* MAX_LEN_FILENAME */
  char str_color[32], comment[2048] = "";
  unsigned long img_width, img_height, map_width, map_height;
  int x, y, xxx, yyy, row, i, index, plrwidth;
  bool withplr = BV_ISSET_ANY(pimg->def->player.plrbv);

  MagickWand *mw;
  PixelIterator *imw;
  PixelWand **pmw, *pw;
  DrawingWand *dw;

  MagickWandGenesis();

  mw = NewMagickWand();
  dw = NewDrawingWand();
  pw = NewPixelWand();

  map_width = pimg->size.x * pimg->def->zoom;
  map_height = pimg->size.y * pimg->def->zoom;

  img_width = map_width + 2 * BORDER_WIDTH;
  img_height = map_height + 2 * BORDER_HEIGHT + TEXT_HEIGHT + SPACER_HEIGHT
               + (withplr ? 2 * SPACER_HEIGHT : 0);

  PixelSetColor(pw, img_bg_color[pimg->def->bg]);
  MagickNewImage(mw, img_width, img_height, pw);

  PixelSetColor(pw, img_fg_color[pimg->def->bg]);
  DrawSetFillColor(dw, pw);
  DrawSetFont(dw, "Times-New-Roman");
  DrawSetFontSize(dw, TEXT_HEIGHT);
  DrawAnnotation(dw, BORDER_WIDTH, TEXT_HEIGHT + BORDER_HEIGHT,
                 (unsigned char *)pimg->title);
  MagickDrawImage(mw, dw);

  if (withplr) {
    /* show a line displaying the colors of alive players */
    plrwidth = map_width / (MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS);
    if (bvplayers_count(pimg->def) == 1) {
      /* only one player */
      for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
        if (BV_ISSET(pimg->def->player.plrbv, i)) {
          pplr_only = player_by_number(i);
          break;
        }
      }
    }

    imw = NewPixelRegionIterator(mw, BORDER_WIDTH,
                                 BORDER_HEIGHT + TEXT_HEIGHT + SPACER_HEIGHT,
                                 map_width, LINE_HEIGHT);
    /* y coordinate */
    for (y = 0; y < LINE_HEIGHT; y++) {
      pmw = PixelGetNextIteratorRow(imw, &map_width);

      /* x coordinate */
      for (x = 0; x < map_width; x++) {
        i = x / plrwidth;
        pplr_now = valid_player_by_number(i);

        if (i > player_count() || pplr_now == NULL || !pplr_now->is_alive) {
          continue;
        }

        if (BV_ISSET(pimg->def->player.plrbv, i)) {
          /* player is alive - display it */
          pcolor = get_player_color(tileset, player_by_number(i));
          SET_COLOR(str_color, pcolor);
          PixelSetColor(pmw[x], str_color);
        } else if (pplr_only != NULL && pplayers_allied(pplr_now, pplr_only)
                   && (x + y) % 2 == 0) {
          /* player is allied to the selected player */
          pcolor = get_player_color(tileset, player_by_number(i));
          SET_COLOR(str_color, pcolor);
          PixelSetColor(pmw[x], str_color);
        }
      }
      PixelSyncIterator(imw);
    }
    DestroyPixelIterator(imw);
  }

  imw = NewPixelRegionIterator(mw, BORDER_WIDTH,
                               BORDER_HEIGHT + TEXT_HEIGHT + SPACER_HEIGHT
                               + (withplr ? (LINE_HEIGHT + SPACER_HEIGHT) : 0),
                               map_width, map_height);
  /* y coordinate */
  for (y = 0; y < pimg->size.y; y++) {
    /* zoom for y */
    for (yyy = 0; yyy < pimg->def->zoom; yyy++) {

      pmw = PixelGetNextIteratorRow(imw, &map_width);

      /* x coordinate */
      for (x = 0; x < pimg->size.x; x++) {
        index = img_index(x, y, pimg);
        pcolor = pimg->map[index];

        if (pcolor != NULL) {
          SET_COLOR(str_color, pcolor);

          /* zoom for x */
          for (xxx = 0; xxx < pimg->def->zoom; xxx++) {
            row = x * pimg->def->zoom + xxx;
            PixelSetColor(pmw[row], str_color);
          }
        }
      }
      PixelSyncIterator(imw);
    }
  }
  DestroyPixelIterator(imw);

  if (BV_ISSET_ANY(pimg->def->player.plrbv)) {
    players_iterate(pplayer) {
      if (!BV_ISSET(pimg->def->player.plrbv, player_index(pplayer))) {
        continue;
      }

      pcolor = get_player_color(tileset, pplayer);
      cat_snprintf(comment, sizeof(comment),
                   "playerno:%d:color:#%02x%02x%02x:name:\"%s\"\n",
                   player_number(pplayer), pcolor->color.red >> 8,
                   pcolor->color.green >> 8, pcolor->color.blue >> 8,
                   player_name(pplayer));
    } players_iterate_end;

    MagickCommentImage(mw, comment);
  }

  my_snprintf(imagefile, sizeof(imagefile), "%s.%s", filename,
              image_extension[pimg->def->imgformat]);
  if (!MagickWriteImage(mw, imagefile)) {
    ret = FALSE;
  } else {
    log_normal("map image saved as '%s'", imagefile);
  }

  DestroyDrawingWand(dw);
  DestroyPixelWand(pw);
  DestroyMagickWand(mw);

  MagickWandTerminus();

  return ret;
}
#undef BORDER_HEIGHT
#undef BORDER_WIDTH
#undef SPACER_HEIGHT
#undef TEXT_HEIGHT
#undef SET_COLOR
#else
static bool img_save(const struct img *pimg, const char *filename)
{
  char ppmname[512]; /* MAX_LEN_FILENAME */
  FILE *fp;
  int x, y, xxx, yyy, index;
  struct color *pcolor;

  my_snprintf(ppmname, sizeof(ppmname), "%s.ppm", filename);
  fp = fopen(ppmname, "w");
  if (!fp) {
    log_error("couldn't open file: %s\n", ppmname);
    return FALSE;
  }

  fprintf(fp, "P3\n# version:2\n# gameid: %s\n", server.game_identifier);
  fprintf(fp, "# An intermediate map from saved Freeciv game %s%+05d\n",
          game.server.save_name, game.info.year);
  fprintf(fp, "# map definition: %s", pimg->def->maparg);

  if (BV_ISSET_ANY(pimg->def->player.plrbv)) {
    players_iterate(pplayer) {
      if (!BV_ISSET(pimg->def->player.plrbv, player_index(pplayer))) {
        continue;
      }

      pcolor = get_player_color(tileset, pplayer);
      fprintf(fp, "# playerno:%d:color:#%02x%02x%02x:name:\"%s\"\n",
              player_number(pplayer), pcolor->color.red >> 8,
              pcolor->color.green >> 8, pcolor->color.blue >> 8,
              player_name(pplayer));
    } players_iterate_end;
  } else {
    fprintf(fp, "# no players");
  }

  fprintf(fp, "%d %d\n", pimg->size.x * pimg->def->zoom,
          pimg->size.y * pimg->def->zoom);
  fprintf(fp, "255\n");

  /* y coordinate */
  for (y = 0; y < pimg->size.y; y++) {
    /* zoom for y */
    for (yyy = 0; yyy < pimg->def->zoom; yyy++) {
      /* x coordinate */
      for (x = 0; x < pimg->size.x; x++) {
        index = img_index(x, y, pimg);
        pcolor = pimg->map[index];

        /* zoom for x */
        for (xxx = 0; xxx < pimg->def->zoom; xxx++) {
          if (pcolor != NULL) {
            fprintf(fp, "%d %d %d\n", pcolor->color.red >> 8,
                   pcolor->color.green >> 8, pcolor->color.blue >> 8);
          } else {
            fprintf(fp, "255 255 255\n"); /* white */
          }
        }
      }
    }
  }

  log_normal("map image saved as '%s'", ppmname);
  fclose(fp);
  return TRUE;
}
#endif /* HAVE_WAND */

/**************************************************************************
  Create the map considering the options (terrain, player(s), cities,
  units, borders, known, fogofwar, ...).
**************************************************************************/
static void img_createmap(struct img *pimg)
{
  struct color *pcolor;
  bv_pixel pixel;
  int x, y, player_id, i;
  struct player *pplayer = NULL;
  enum known_type tile_knowledge = TILE_UNKNOWN;

  whole_map_iterate(ptile) {
    x = ptile->x;
    y = ptile->y;

    if (bvplayers_count(pimg->def) == 1) {
      /* only one player; get player id for 'known' and 'fogofwar' */
      for (i = 0; i < player_count(); i++) {
        if (BV_ISSET(pimg->def->player.plrbv, i)) {
          pplayer = player_by_number(i);
          tile_knowledge = tile_get_known(ptile, pplayer);
          break;
        }
      }
    }

    /* known tiles */
    if (pimg->def->known && pplayer != NULL && tile_knowledge == TILE_UNKNOWN) {
      /* plot nothing if tile is not known */
      continue;
    }

    /* terrain */
    if (pimg->def->terrain) {
      /* full terrain */
      pixel = pimg->pixel_tile(x, y);
      pcolor = get_terrain_color(tileset, ptile->terrain);
      img_plot(pimg, x, y, pcolor, pixel);
    } else {
      /* basic terrain */
      pixel = pimg->pixel_tile(x, y);
      if (is_ocean_tile(ptile)) {
        img_plot(pimg, x, y, get_color(tileset, COLOR_OVERVIEW_OCEAN),
                 pixel);
      } else {
        img_plot(pimg, x, y, get_color(tileset, COLOR_OVERVIEW_LAND),
                 pixel);
      }
    }

    /* cities and units */
    if (pimg->def->cities && tile_city(ptile)) {
      player_id = player_index(city_owner(tile_city(ptile)));
      if (BV_ISSET(pimg->def->player.plrbv, player_id)
          || (pplayer != NULL && tile_knowledge == TILE_KNOWN_SEEN)) {
        /* plot cities if player is selected or view range of the one
         * displayed player*/
        pixel = pimg->pixel_city(x, y);
        pcolor = get_player_color(tileset, player_by_number(player_id));
        img_plot(pimg, x, y, pcolor, pixel);
      }
    } else if (pimg->def->units && unit_list_size(ptile->units) > 0) {
      player_id = player_index(unit_owner(unit_list_get(ptile->units, 0)));
      if (BV_ISSET(pimg->def->player.plrbv, player_id)
          || (pplayer != NULL && tile_knowledge == TILE_KNOWN_SEEN)) {
        /* plot units if player is selected or view range of the one
         * displayed player*/
        pixel = pimg->pixel_unit(x, y);
        pcolor = get_player_color(tileset, player_by_number(player_id));
        img_plot(pimg, x, y, pcolor, pixel);
      }
    }

    /* (land) area within borders and borders */
    if (game.info.borders > 0 && NULL != tile_owner(ptile)) {
      player_id = player_index(tile_owner(ptile));
      if (pimg->def->area && !is_ocean_tile(ptile)
          && BV_ISSET(pimg->def->player.plrbv, player_id)) {
        /* the tile is land and inside the players borders */
        pixel = pimg->pixel_tile(x, y);
        pcolor = get_player_color(tileset, player_by_number(player_id));
        img_plot(pimg, x, y, pcolor, pixel);
      } else if (pimg->def->borders
                 && (BV_ISSET(pimg->def->player.plrbv, player_id)
                     || (pplayer != NULL
                         && tile_knowledge == TILE_KNOWN_SEEN))) {
        /* plot borders if player is selected or view range of the one
         * displayed player*/
        pixel = pimg->pixel_border(x, y);
        pcolor = get_player_color(tileset, player_by_number(player_id));
        img_plot(pimg, x, y, pcolor, pixel);
      }
    }

    /* fogofwar; if only 1 player is plotted */
    if (game.info.fogofwar && pimg->def->fogofwar && pplayer != NULL
        && tile_knowledge == TILE_KNOWN_UNSEEN) {
      pixel = pimg->pixel_fogofwar(x, y);
      pcolor = NULL;
      img_plot(pimg, x, y, pcolor, pixel);
    }

  } whole_map_iterate_end;
}

/*
 * ==============================================
 * topology functions
 * ==============================================
 * With these functions the pixels corresponding to the different elements
 * (tile, city, unit) for each map topology are defined.
 *
 * The bv_pixel_fogofwar_*() functions are special as they defines where
 * the color should be removed.
 *
 * The functions for a rectangular and an isometric rectangular topology
 * are identical.
 */

/**************************************************************************
   0  1  2  3  3  5
   6  7  8  9 10 11
  12 13 14 15 16 17
  18 19 20 21 22 23
  24 25 26 27 28 29
  30 31 32 33 34 35
**************************************************************************/
static bv_pixel pixel_tile_rect(const int x, const int y)
{
  bv_pixel pixel;

  BV_SET_ALL(pixel);

  return pixel;
}

/**************************************************************************
  -- -- -- -- -- --
  --  7  8  9 10 --
  -- 13 14 15 16 --
  -- 19 20 21 22 --
  -- 25 26 27 28 --
  -- -- -- -- -- --
**************************************************************************/
static bv_pixel pixel_city_rect(const int x, const int y)
{
  bv_pixel pixel;

  BV_CLR_ALL(pixel);
  BV_SET(pixel, 7);
  BV_SET(pixel, 8);
  BV_SET(pixel, 9);
  BV_SET(pixel, 10);
  BV_SET(pixel, 13);
  BV_SET(pixel, 14);
  BV_SET(pixel, 15);
  BV_SET(pixel, 16);
  BV_SET(pixel, 19);
  BV_SET(pixel, 20);
  BV_SET(pixel, 21);
  BV_SET(pixel, 22);
  BV_SET(pixel, 21);
  BV_SET(pixel, 25);
  BV_SET(pixel, 26);
  BV_SET(pixel, 27);
  BV_SET(pixel, 28);

  return pixel;
}

/**************************************************************************
  -- -- -- -- -- --
  -- -- -- -- -- --
  -- -- 14 15 -- --
  -- -- 20 21 -- --
  -- -- -- -- -- --
  -- -- -- -- -- --
**************************************************************************/
static bv_pixel pixel_unit_rect(const int x, const int y)
{
  bv_pixel pixel;

  BV_CLR_ALL(pixel);
  BV_SET(pixel, 14);
  BV_SET(pixel, 15);
  BV_SET(pixel, 20);
  BV_SET(pixel, 21);

  return pixel;
}

/**************************************************************************
   0 --  2 --  4 --
  --  7 --  9 -- 11
  12 -- 14 -- 16 --
  -- 19 -- 21 -- 23
  24 -- 26 -- 28 --
  -- 31 -- 33 -- 35
**************************************************************************/
static bv_pixel pixel_fogofwar_rect(const int x, const int y)
{
  bv_pixel pixel;

  BV_CLR_ALL(pixel);

  BV_SET(pixel,  0);
  BV_SET(pixel,  2);
  BV_SET(pixel,  4);
  BV_SET(pixel,  7);
  BV_SET(pixel,  9);
  BV_SET(pixel, 11);
  BV_SET(pixel, 12);
  BV_SET(pixel, 14);
  BV_SET(pixel, 16);
  BV_SET(pixel, 19);
  BV_SET(pixel, 21);
  BV_SET(pixel, 23);
  BV_SET(pixel, 24);
  BV_SET(pixel, 26);
  BV_SET(pixel, 28);
  BV_SET(pixel, 31);
  BV_SET(pixel, 33);
  BV_SET(pixel, 35);

  return pixel;
}

/**************************************************************************
             [N]

       0  1  2  3  3  5
       6 -- -- -- -- 11
  [W] 12 -- -- -- -- 17 [E]
      18 -- -- -- -- 23
      24 -- -- -- -- 29
      30 31 32 33 34 35

             [S]
**************************************************************************/
static bv_pixel pixel_border_rect(const int x, const int y)
{
  bv_pixel pixel;
  struct tile *ptile, *pcenter;
  struct player *owner;

  BV_CLR_ALL(pixel);

  pcenter = map_pos_to_tile(x, y);
  if (NULL == pcenter) {
    /* no tile */
    return pixel;
  }

  owner = tile_owner(pcenter);
  if (NULL == owner) {
    /* no border */
    return pixel;
  }

  ptile = mapstep(pcenter, DIR8_NORTH);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 0);
    BV_SET(pixel, 1);
    BV_SET(pixel, 2);
    BV_SET(pixel, 3);
    BV_SET(pixel, 4);
    BV_SET(pixel, 5);
  }

  ptile = mapstep(pcenter, DIR8_EAST);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 5);
    BV_SET(pixel, 11);
    BV_SET(pixel, 17);
    BV_SET(pixel, 23);
    BV_SET(pixel, 29);
    BV_SET(pixel, 35);
  }

  ptile = mapstep(pcenter, DIR8_SOUTH);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 30);
    BV_SET(pixel, 31);
    BV_SET(pixel, 32);
    BV_SET(pixel, 33);
    BV_SET(pixel, 34);
    BV_SET(pixel, 35);
  }

  ptile = mapstep(pcenter, DIR8_WEST);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 0);
    BV_SET(pixel, 6);
    BV_SET(pixel, 12);
    BV_SET(pixel, 18);
    BV_SET(pixel, 24);
    BV_SET(pixel, 30);
  }

  return pixel;
}

/**************************************************************************
  Base coordinates for the tiles on a (isometric) rectange topology,
**************************************************************************/
static void base_coor_rect(int *base_x, int *base_y, int x, int y)
{
  *base_x = x * TILE_SIZE;
  *base_y = y * TILE_SIZE;
}

/**************************************************************************
         0  1
      2  3  4  5
   6  7  8  9 10 11
  12 13 14 15 16 17
  18 19 20 21 22 23
  24 25 26 27 28 29
     30 31 32 33
        34 35
**************************************************************************/
static bv_pixel pixel_tile_hexa(const int x, const int y)
{
  bv_pixel pixel;

  BV_SET_ALL(pixel);

  return pixel;
}

/**************************************************************************
        -- --
     --  3  4 --
  --  7  8  9 10 --
  -- 13 14 15 16 --
  -- 19 20 21 22 --
  -- 25 26 27 28 --
     -- 31 32 --
        -- --
**************************************************************************/
static bv_pixel pixel_city_hexa(const int x, const int y)
{
  bv_pixel pixel;

  BV_CLR_ALL(pixel);
  BV_SET(pixel, 3);
  BV_SET(pixel, 4);
  BV_SET(pixel, 7);
  BV_SET(pixel, 8);
  BV_SET(pixel, 9);
  BV_SET(pixel, 10);
  BV_SET(pixel, 13);
  BV_SET(pixel, 14);
  BV_SET(pixel, 15);
  BV_SET(pixel, 16);
  BV_SET(pixel, 19);
  BV_SET(pixel, 20);
  BV_SET(pixel, 21);
  BV_SET(pixel, 22);
  BV_SET(pixel, 25);
  BV_SET(pixel, 26);
  BV_SET(pixel, 27);
  BV_SET(pixel, 28);
  BV_SET(pixel, 31);
  BV_SET(pixel, 32);

  return pixel;
}

/**************************************************************************
        -- --
     -- -- -- --
  -- -- -- -- -- --
  -- -- 14 15 -- --
  -- -- 20 21 -- --
  -- -- -- -- -- --
     -- -- -- --
        -- --
**************************************************************************/
static bv_pixel pixel_unit_hexa(const int x, const int y)
{
  bv_pixel pixel;

  BV_CLR_ALL(pixel);
  BV_SET(pixel, 14);
  BV_SET(pixel, 15);
  BV_SET(pixel, 20);
  BV_SET(pixel, 21);

  return pixel;
}

/**************************************************************************
         0 --
     --  3 --  5
  --  7 --  9 -- 11
  -- 13 -- 15 -- 17
  18 -- 20 -- 22 --
  24 -- 26 -- 28 --
     30 -- 32 --
        -- 35
**************************************************************************/
static bv_pixel pixel_fogofwar_hexa(const int x, const int y)
{
  bv_pixel pixel;

  BV_CLR_ALL(pixel);
  BV_SET(pixel,  0);
  BV_SET(pixel,  3);
  BV_SET(pixel,  5);
  BV_SET(pixel,  7);
  BV_SET(pixel,  9);
  BV_SET(pixel, 11);
  BV_SET(pixel, 13);
  BV_SET(pixel, 15);
  BV_SET(pixel, 17);
  BV_SET(pixel, 18);
  BV_SET(pixel, 20);
  BV_SET(pixel, 22);
  BV_SET(pixel, 24);
  BV_SET(pixel, 26);
  BV_SET(pixel, 28);
  BV_SET(pixel, 30);
  BV_SET(pixel, 32);
  BV_SET(pixel, 35);

  return pixel;
}

/**************************************************************************
   [W]        0  1       [N]
           2 -- --  5
        6 -- -- -- -- 11
  [SW] 12 -- -- -- -- 17 [NE]
       18 -- -- -- -- 23
       24 -- -- -- -- 29
          30 -- -- 33
   [S]       34 35       [E]
**************************************************************************/
static bv_pixel pixel_border_hexa(const int x, const int y)
{
  bv_pixel pixel;
  struct tile *ptile, *pcenter;
  struct player *owner;

  BV_CLR_ALL(pixel);

  pcenter = map_pos_to_tile(x, y);
  if (NULL == pcenter) {
    /* no tile */
    return pixel;
  }

  owner = tile_owner(pcenter);
  if (NULL == owner) {
    /* no border */
    return pixel;
  }

  ptile = mapstep(pcenter, DIR8_WEST);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 0);
    BV_SET(pixel, 2);
    BV_SET(pixel, 6);
  }

  /* not used: DIR8_NORTHWEST */

  ptile = mapstep(pcenter, DIR8_NORTH);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 1);
    BV_SET(pixel, 5);
    BV_SET(pixel, 11);
  }

  ptile = mapstep(pcenter, DIR8_NORTHEAST);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 11);
    BV_SET(pixel, 17);
    BV_SET(pixel, 23);
    BV_SET(pixel, 29);
  }

  ptile = mapstep(pcenter, DIR8_EAST);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 29);
    BV_SET(pixel, 33);
    BV_SET(pixel, 35);
  }

  /* not used. DIR8_SOUTHEAST */

  ptile = mapstep(pcenter, DIR8_SOUTH);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 24);
    BV_SET(pixel, 30);
    BV_SET(pixel, 34);
  }

  ptile = mapstep(pcenter, DIR8_SOUTHWEST);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 6);
    BV_SET(pixel, 12);
    BV_SET(pixel, 18);
    BV_SET(pixel, 24);
  }

  return pixel;
}

/**************************************************************************
  Base coordinates for the tiles on a hexa topology,
**************************************************************************/
static void base_coor_hexa(int *base_x, int *base_y, int x, int y)
{
  int nat_x, nat_y;
  MAP_TO_NATIVE_POS(&nat_x, &nat_y, x, y);

  *base_x = nat_x * TILE_SIZE + ((nat_y % 2) ? TILE_SIZE / 2 : 0);
  *base_y = nat_y * TILE_SIZE;
}

/**************************************************************************
         0  1  2  3
      4  5  6  7  8  9
  10 11 12 13 14 15 16 17
  18 19 20 21 22 23 24 25
     26 27 28 29 30 31
        32 33 34 35
**************************************************************************/
static bv_pixel pixel_tile_isohexa(const int x, const int y)
{
  bv_pixel pixel;

  BV_SET_ALL(pixel);

  return pixel;
}

/**************************************************************************
        -- -- -- --
     --  5  6  7  8 --
  -- 11 12 13 14 15 16 --
  -- 19 20 21 22 23 24 --
     -- 27 28 29 30 --
        -- -- -- --
**************************************************************************/
static bv_pixel pixel_city_isohexa(const int x, const int y)
{
  bv_pixel pixel;

  BV_CLR_ALL(pixel);
  BV_SET(pixel, 5);
  BV_SET(pixel, 6);
  BV_SET(pixel, 7);
  BV_SET(pixel, 8);
  BV_SET(pixel, 11);
  BV_SET(pixel, 12);
  BV_SET(pixel, 13);
  BV_SET(pixel, 14);
  BV_SET(pixel, 15);
  BV_SET(pixel, 16);
  BV_SET(pixel, 19);
  BV_SET(pixel, 20);
  BV_SET(pixel, 21);
  BV_SET(pixel, 22);
  BV_SET(pixel, 23);
  BV_SET(pixel, 24);
  BV_SET(pixel, 27);
  BV_SET(pixel, 28);
  BV_SET(pixel, 29);
  BV_SET(pixel, 30);

  return pixel;
}

/**************************************************************************
        -- -- -- --
     -- -- -- -- -- --
  -- -- -- 13 14 -- -- --
  -- -- -- 21 22 -- -- --
     -- -- -- -- -- --
        -- -- -- --
**************************************************************************/
static bv_pixel pixel_unit_isohexa(const int x, const int y)
{
  bv_pixel pixel;

  BV_CLR_ALL(pixel);
  BV_SET(pixel, 13);
  BV_SET(pixel, 14);
  BV_SET(pixel, 21);
  BV_SET(pixel, 22);

  return pixel;
}

/**************************************************************************
         0  1 -- --
      4 -- --  7  8 --
  -- -- 12 13 -- -- 16 17
  18 19 -- -- 22 23 -- --
     -- 27 28 -- -- 31
        -- -- 34 35
**************************************************************************/
static bv_pixel pixel_fogofwar_isohexa(const int x, const int y)
{
  bv_pixel pixel;

  BV_CLR_ALL(pixel);
  BV_SET(pixel,  0);
  BV_SET(pixel,  1);
  BV_SET(pixel,  4);
  BV_SET(pixel,  7);
  BV_SET(pixel,  8);
  BV_SET(pixel, 12);
  BV_SET(pixel, 13);
  BV_SET(pixel, 16);
  BV_SET(pixel, 17);
  BV_SET(pixel, 18);
  BV_SET(pixel, 19);
  BV_SET(pixel, 22);
  BV_SET(pixel, 23);
  BV_SET(pixel, 27);
  BV_SET(pixel, 28);
  BV_SET(pixel, 31);
  BV_SET(pixel, 34);
  BV_SET(pixel, 35);

  return pixel;
}

/**************************************************************************

               [N]

  [NW]       0  1  2  3      [E]
          4 -- -- -- --  9
      10 -- -- -- -- -- -- 17
      18 -- -- -- -- -- -- 25
         26 -- -- -- -- 31
  [W]       32 33 34 35      [SE]

               [S]
**************************************************************************/
static bv_pixel pixel_border_isohexa(const int x, const int y)
{
  bv_pixel pixel;
  struct tile *ptile, *pcenter;
  struct player *owner;

  BV_CLR_ALL(pixel);

  pcenter = map_pos_to_tile(x, y);
  if (NULL == pcenter) {
    /* no tile */
    return pixel;
  }

  owner = tile_owner(pcenter);
  if (NULL == owner) {
    /* no border */
    return pixel;
  }

  ptile = mapstep(pcenter, DIR8_NORTH);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 0);
    BV_SET(pixel, 1);
    BV_SET(pixel, 2);
    BV_SET(pixel, 3);
  }

  /* not used: DIR8_NORTHEAST */

  ptile = mapstep(pcenter, DIR8_EAST);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 3);
    BV_SET(pixel, 9);
    BV_SET(pixel, 17);
  }

  ptile = mapstep(pcenter, DIR8_SOUTHEAST);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 25);
    BV_SET(pixel, 31);
    BV_SET(pixel, 35);
  }

  ptile = mapstep(pcenter, DIR8_SOUTH);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 32);
    BV_SET(pixel, 33);
    BV_SET(pixel, 34);
    BV_SET(pixel, 35);
  }

  /* not used: DIR8_SOUTHWEST */

  ptile = mapstep(pcenter, DIR8_WEST);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 18);
    BV_SET(pixel, 26);
    BV_SET(pixel, 32);
  }

  ptile = mapstep(pcenter, DIR8_NORTHWEST);
  if (!ptile || tile_owner(ptile) != owner) {
    BV_SET(pixel, 0);
    BV_SET(pixel, 4);
    BV_SET(pixel, 10);
  }

  return pixel;
}

/**************************************************************************
  Base coordinates for the tiles on a isometric hexa topology,
**************************************************************************/
static void base_coor_isohexa(int *base_x, int *base_y, int x, int y)
{
  /* magic for iso-hexa */
  y -= x / 2;
  y += (map.xsize - 1)/2;

  *base_x = x * TILE_SIZE;
  *base_y = y * TILE_SIZE + ((x % 2) ? 0 : TILE_SIZE / 2);
}

/*
 * ==============================================
 * additonal functions
 * ==============================================
 */

/**************************************************************************
  Generate a file name for a map image.

  <savename>-T<turn>-Y<year>-<mapstr>.<mapext>

  <savename>-T<turn>-Y<year>: identical to filename for savegames
  <mapstr>:                   map string as defined by this function
                              M<map options>Z<zoom factor>P<player bitvector>
                              For the player bitvector all 32 values are used
                              due to the possibility of additional players
                              during the game (civil war, barbarians).
  <mapext>:                   image extension
**************************************************************************/
static int generate_img_name(char *buf, int buflen,
                             const struct mapdef *pmapdef,
                             const char *basename)
{
  int nb;

  nb = my_snprintf(buf, buflen, "%s-M%c%c%c%c%c%c%cZ%dP%s",
                   basename, pmapdef->area ? 'a' : '-',
                   pmapdef->borders ? 'b' : '-', pmapdef->cities ? 'c' : '-',
                   pmapdef->fogofwar ? 'f' : '-', pmapdef->known ? 'k' : '-',
                   pmapdef->units ? 'u' : '-', pmapdef->terrain ? 't' : '-',
                   pmapdef->zoom, bvplayers_str(pmapdef));

  return nb;
}

/**************************************************************************
  Convert the player bitvector to a string.
**************************************************************************/
static char *bvplayers_str(const struct mapdef *pmapdef)
{
  static char buf[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS + 1];
  int i;

  switch (pmapdef->player.show) {
  case SHOW_PLRNAME:
  case SHOW_PLRID:
    MAPIMG_RETURN_VAL_IF_FAIL((pmapdef->player.show != SHOW_PLRNAME
                               || pmapdef->player.show != SHOW_PLRID),
                              "error");
    break;
  case SHOW_NONE:
    /* no player on the map */
    sz_strlcpy(buf, "none");
    break;
  case SHOW_PLRBV:
  case SHOW_EACH:
    /* one map for each selected player; iterate over all posible player ids
     * to generate unique strings even if civil wars occur */
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      buf[i] = BV_ISSET(pmapdef->player.plrbv, i) ? '1' : '0';
    }
    buf[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS] = '\0';
    break;
  case SHOW_ALL:
    /* show all players in one map */
    sz_strlcpy(buf, "all");
    break;
  }

  return buf;
}

/**************************************************************************
  Return the number of players defined in a map image definition.
**************************************************************************/
static int bvplayers_count(const struct mapdef *pmapdef)
{
  int i, count = 0;

  if (!game_was_started_todo()) {
    /* game was never started */
    return 0;
  }

  switch (pmapdef->player.show) {
  case SHOW_NONE:    /* no player on the map */
    count = 0;
    break;
  case SHOW_PLRNAME: /* the map of one selected player */
  case SHOW_PLRID:
    count = 1;
    break;
  case SHOW_PLRBV:   /* map showing only players given by a bitvector */
    count = 0;
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      if (BV_ISSET(pmapdef->player.plrbv, i)) {
        count++;
      }
    }
    break;
  case SHOW_EACH:    /* one map for each player */
    count = 1;
    break;
  case SHOW_ALL:     /* show all players in one map */
    count = player_count();
    break;
  }

  return count;
}

static bool game_was_started_todo(void)
{
  return TRUE;
}
