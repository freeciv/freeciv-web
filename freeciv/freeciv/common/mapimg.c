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
#include <fc_config.h>
#endif

#include <stdarg.h>

#ifdef HAVE_MAPIMG_MAGICKWAND
  #include <wand/MagickWand.h>
#endif /* HAVE_MAPIMG_MAGICKWAND */

/* utility */
#include "astring.h"
#include "bitvector.h"
#include "fc_cmdline.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "string_vector.h"
#include "timing.h"

/* common */
#include "calendar.h"
#include "connection.h"
#include "fc_types.h"
#include "game.h"
#include "map.h"
#include "rgbcolor.h"
#include "terrain.h"
#include "tile.h"
#include "version.h"
#include "player.h"

#include "mapimg.h"

/* Some magick for ImageMagick - the interface has changed:
     ImageMagick-6.6.2-0: PixelGetNextIteratorRow(..., unsigned long *)
     ImageMagick-6.6.2-1: PixelGetNextIteratorRow(..., size_t *)
   Theoretically, "unsigned long" and "size_t" are pretty much the same but
   in practice the compiler will complain bitterly.
   (from Gem-0.93 ImageMAGICK plugin) */
#ifndef MagickLibInterface
# define MagickLibInterface 0
#endif
#ifndef MagickLibVersion
# define MagickLibVersion 0
#endif

/* This won't catch ImageMagick>=6.6.2-0.
   Another workaround: compile with "-fpermissive" */
#if (MagickLibInterface > 3) || (MagickLibVersion >= 0x662)
# define magickwand_size_t size_t
#else
# define magickwand_size_t unsigned long
#endif

/* == image colors == */
enum img_special {
  IMGCOLOR_ERROR,
  IMGCOLOR_OCEAN,
  IMGCOLOR_GROUND,
  IMGCOLOR_BACKGROUND,
  IMGCOLOR_TEXT
};

static const struct rgbcolor *imgcolor_special(enum img_special imgcolor);
static const struct rgbcolor *imgcolor_player(int plr_id);
static const struct rgbcolor
  *imgcolor_terrain(const struct terrain *pterrain);

/* == topologies == */
#define TILE_SIZE 6
#define NUM_PIXEL TILE_SIZE * TILE_SIZE

BV_DEFINE(bv_pixel, NUM_PIXEL);

struct tile_shape {
  int x[NUM_PIXEL];
  int y[NUM_PIXEL];
};

struct img;

typedef bv_pixel (*plot_func)(const struct tile *ptile,
                              const struct player *pplayer,
                              bool knowledge);
typedef void (*base_coor_func)(struct img *pimg, int *base_x, int *base_y,
                               int x, int y);

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

static bv_pixel pixel_tile_rect(const struct tile *ptile,
                                const struct player *pplayer,
                                bool knowledge);
static bv_pixel pixel_city_rect(const struct tile *ptile,
                                const struct player *pplayer,
                                bool knowledge);
static bv_pixel pixel_unit_rect(const struct tile *ptile,
                                const struct player *pplayer,
                                bool knowledge);
static bv_pixel pixel_fogofwar_rect(const struct tile *ptile,
                                    const struct player *pplayer,
                                    bool knowledge);
static bv_pixel pixel_border_rect(const struct tile *ptile,
                                  const struct player *pplayer,
                                  bool knowledge);
static void base_coor_rect(struct img *pimg, int *base_x, int *base_y,
                           int x, int y);

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

static bv_pixel pixel_tile_hexa(const struct tile *ptile,
                                const struct player *pplayer,
                                bool knowledge);
static bv_pixel pixel_city_hexa(const struct tile *ptile,
                                const struct player *pplayer,
                                bool knowledge);
static bv_pixel pixel_unit_hexa(const struct tile *ptile,
                                const struct player *pplayer,
                                bool knowledge);
static bv_pixel pixel_fogofwar_hexa(const struct tile *ptile,
                                    const struct player *pplayer,
                                    bool knowledge);
static bv_pixel pixel_border_hexa(const struct tile *ptile,
                                  const struct player *pplayer,
                                  bool knowledge);
static void base_coor_hexa(struct img *pimg, int *base_x, int *base_y,
                           int x, int y);

/* isometric hexa topology */
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

static bv_pixel pixel_tile_isohexa(const struct tile *ptile,
                                   const struct player *pplayer,
                                   bool knowledge);
static bv_pixel pixel_city_isohexa(const struct tile *ptile,
                                   const struct player *pplayer,
                                   bool knowledge);
static bv_pixel pixel_unit_isohexa(const struct tile *ptile,
                                   const struct player *pplayer,
                                   bool knowledge);
static bv_pixel pixel_fogofwar_isohexa(const struct tile *ptile,
                                       const struct player *pplayer,
                                       bool knowledge);
static bv_pixel pixel_border_isohexa(const struct tile *ptile,
                                     const struct player *pplayer,
                                     bool knowledge);
static void base_coor_isohexa(struct img *pimg, int *base_x, int *base_y,
                              int x, int y);

/* == map images == */

#define ARG_PLRBV   "plrbv"
#define ARG_PLRID   "plrid"
#define ARG_PLRNAME "plrname"

/* mapimg definition */
#define SPECENUM_NAME mapdef_arg
#define SPECENUM_VALUE0     MAPDEF_FORMAT
#define SPECENUM_VALUE0NAME "format"
#define SPECENUM_VALUE1     MAPDEF_MAP
#define SPECENUM_VALUE1NAME "map"
#define SPECENUM_VALUE2     MAPDEF_PLRBV
#define SPECENUM_VALUE2NAME ARG_PLRBV
#define SPECENUM_VALUE3     MAPDEF_PLRID
#define SPECENUM_VALUE3NAME ARG_PLRID
#define SPECENUM_VALUE4     MAPDEF_PLRNAME
#define SPECENUM_VALUE4NAME ARG_PLRNAME
#define SPECENUM_VALUE5     MAPDEF_SHOW
#define SPECENUM_VALUE5NAME "show"
#define SPECENUM_VALUE6     MAPDEF_TURNS
#define SPECENUM_VALUE6NAME "turns"
#define SPECENUM_VALUE7     MAPDEF_ZOOM
#define SPECENUM_VALUE7NAME "zoom"
#define SPECENUM_COUNT      MAPDEF_COUNT
#include "specenum_gen.h"

BV_DEFINE(bv_mapdef_arg, MAPDEF_COUNT);

/* image format */
#define SPECENUM_NAME imageformat
#define SPECENUM_BITWISE
#define SPECENUM_VALUE0     IMGFORMAT_GIF
#define SPECENUM_VALUE0NAME "gif"
#define SPECENUM_VALUE1     IMGFORMAT_PNG
#define SPECENUM_VALUE1NAME "png"
#define SPECENUM_VALUE2     IMGFORMAT_PPM
#define SPECENUM_VALUE2NAME "ppm"
#define SPECENUM_VALUE3     IMGFORMAT_JPG
#define SPECENUM_VALUE3NAME "jpg"
#include "specenum_gen.h"

/* image format */
#define SPECENUM_NAME imagetool
#define SPECENUM_VALUE0     IMGTOOL_PPM
#define SPECENUM_VALUE0NAME "ppm"
#define SPECENUM_VALUE1     IMGTOOL_MAGICKWAND
#define SPECENUM_VALUE1NAME "magick"
#include "specenum_gen.h"

/* player definitions */
#define SPECENUM_NAME show_player
#define SPECENUM_VALUE0     SHOW_NONE
#define SPECENUM_VALUE0NAME "none"
#define SPECENUM_VALUE1     SHOW_EACH
#define SPECENUM_VALUE1NAME "each"
#define SPECENUM_VALUE2     SHOW_HUMAN
#define SPECENUM_VALUE2NAME "human"
#define SPECENUM_VALUE3     SHOW_ALL
#define SPECENUM_VALUE3NAME "all"
/* should be identical to MAPDEF_PLRNAME */
#define SPECENUM_VALUE4     SHOW_PLRNAME
#define SPECENUM_VALUE4NAME ARG_PLRNAME
/* should be identical to MAPDEF_PLRID */
#define SPECENUM_VALUE5     SHOW_PLRID
#define SPECENUM_VALUE5NAME ARG_PLRID
/* should be identical to MAPDEF_PLRBV */
#define SPECENUM_VALUE6     SHOW_PLRBV
#define SPECENUM_VALUE6NAME ARG_PLRBV
#include "specenum_gen.h"

#undef ARG_PLRBV
#undef ARG_PLRID
#undef ARG_PLRNAME

/* map definition status */
#define SPECENUM_NAME mapimg_status
#define SPECENUM_VALUE0     MAPIMG_STATUS_UNKNOWN
#define SPECENUM_VALUE0NAME _("not checked")
#define SPECENUM_VALUE1     MAPIMG_STATUS_OK
#define SPECENUM_VALUE1NAME _("OK")
#define SPECENUM_VALUE2     MAPIMG_STATUS_ERROR
#define SPECENUM_VALUE2NAME _("error")
#include "specenum_gen.h"

#define MAX_LEN_MAPARG MAX_LEN_MAPDEF
#define MAX_NUM_MAPIMG 10

static inline bool mapimg_initialised(void);
static bool mapimg_test(int id);
static bool mapimg_define_arg(struct mapdef *pmapdef, enum mapdef_arg arg,
                              const char *val, bool check);
static bool mapimg_def2str(struct mapdef *pmapdef, char *str, size_t str_len);
static bool mapimg_checkplayers(struct mapdef *pmapdef, bool recheck);
static char *mapimg_generate_name(struct mapdef *pmapdef);

/* == map definition == */
struct mapdef {
  char maparg[MAX_LEN_MAPARG];
  char error[MAX_LEN_MAPDEF];
  enum mapimg_status status;
  enum imageformat format;
  enum imagetool tool;
  int zoom;
  int turns;
  bool layers[MAPIMG_LAYER_COUNT];
  struct {
    enum show_player show;
    union {
      char name[MAX_LEN_NAME]; /* used by SHOW_PLRNAME */
      int id;                  /* used by SHOW_PLRID */
      bv_player plrbv;         /* used by SHOW_PLRBV */
    };
    bv_player checked_plrbv;   /* player bitvector used for the image
                                * creation */
  } player;

  bv_mapdef_arg args;
  bool colortest;
};

static struct mapdef *mapdef_new(bool colortest);
static void mapdef_destroy(struct mapdef *pmapdef);

/* List of map definitions. */
#define SPECLIST_TAG mapdef
#define SPECLIST_TYPE struct mapdef
#include "speclist.h"

#define mapdef_list_iterate(mapdef_list, pmapdef) \
  TYPED_LIST_ITERATE(struct mapdef, mapdef_list, pmapdef)
#define mapdef_list_iterate_end \
  LIST_ITERATE_END

/* == images == */
struct mapdef;

/* Some lengths used for the images created by the magickwand toolkit. */
#define IMG_BORDER_HEIGHT 5
#define IMG_BORDER_WIDTH IMG_BORDER_HEIGHT
#define IMG_SPACER_HEIGHT 5
#define IMG_LINE_HEIGHT 5
#define IMG_TEXT_HEIGHT 12

struct img {
  struct mapdef *def; /* map definition */
  int turn; /* save turn */
  char title[MAX_LEN_MAPDEF];

  /* topology definition */
  struct tile_shape *tileshape;
  plot_func pixel_tile;
  plot_func pixel_city;
  plot_func pixel_unit;
  plot_func pixel_fogofwar;
  plot_func pixel_border;
  base_coor_func base_coor;

  struct {
    int x;
    int y;
  } mapsize; /* map size */
  struct {
    int x;
    int y;
  } imgsize; /* image size */
  const struct rgbcolor **map;
};

static struct img *img_new(struct mapdef *mapdef, int topo, int xsize, int ysize);
static void img_destroy(struct img *pimg);
static inline void img_set_pixel(struct img *pimg, const int mindex,
                                 const struct rgbcolor *pcolor);
static inline int img_index(const int x, const int y,
                            const struct img *pimg);
static const char *img_playerstr(const struct player *pplayer);
static void img_plot(struct img *pimg, int x, int y,
                     const struct rgbcolor *pcolor, const bv_pixel pixel);
static void img_plot_tile(struct img *pimg, const struct tile *ptile,
                          const struct rgbcolor *pcolor, const bv_pixel pixel);
static bool img_save(const struct img *pimg, const char *mapimgfile,
                     const char *path);
static bool img_save_ppm(const struct img *pimg, const char *mapimgfile);
#ifdef HAVE_MAPIMG_MAGICKWAND
static bool img_save_magickwand(const struct img *pimg,
                                const char *mapimgfile);
#endif /* HAVE_MAPIMG_MAGICKWAND */
static bool img_filename(const char *mapimgfile, enum imageformat format,
                         char *filename, size_t filename_len);
static void img_createmap(struct img *pimg);

/* == image toolkits == */
typedef bool (*img_save_func)(const struct img *pimg,
                              const char *mapimgfile);

struct toolkit {
  enum imagetool tool;
  enum imageformat format_default;
  int formats;
  const img_save_func img_save;
  const char *help;
};

#define GEN_TOOLKIT(_tool, _format_default, _formats, _save_func, _help)    \
  {_tool, _format_default, _formats, _save_func, _help},

static struct toolkit img_toolkits[] = {
  GEN_TOOLKIT(IMGTOOL_PPM, IMGFORMAT_PPM, IMGFORMAT_PPM,
              img_save_ppm,
              N_("Standard ppm files"))
#ifdef HAVE_MAPIMG_MAGICKWAND
  GEN_TOOLKIT(IMGTOOL_MAGICKWAND, IMGFORMAT_GIF,
              IMGFORMAT_GIF + IMGFORMAT_PNG + IMGFORMAT_PPM + IMGFORMAT_JPG,
              img_save_magickwand,
              N_("ImageMagick"))
#endif /* HAVE_MAPIMG_MAGICKWAND */
};

static const int img_toolkits_count = ARRAY_SIZE(img_toolkits);

#ifdef HAVE_MAPIMG_MAGICKWAND
  #define MAPIMG_DEFAULT_IMGFORMAT IMGFORMAT_GIF
  #define MAPIMG_DEFAULT_IMGTOOL   IMGTOOL_MAGICKWAND
#else
  #define MAPIMG_DEFAULT_IMGFORMAT IMGFORMAT_PPM
  #define MAPIMG_DEFAULT_IMGTOOL   IMGTOOL_PPM
#endif /* HAVE_MAPIMG_MAGICKWAND */

static const struct toolkit *img_toolkit_get(enum imagetool tool);

#define img_toolkit_iterate(_toolkit)                                       \
  {                                                                         \
    int _i;                                                                 \
    for (_i = 0; _i < img_toolkits_count; _i++) {                           \
      const struct toolkit *_toolkit = &img_toolkits[_i];

#define img_toolkit_iterate_end                                             \
    }                                                                       \
  }

/* == logging == */
#define MAX_LEN_ERRORBUF 1024

static char error_buffer[MAX_LEN_ERRORBUF] = "\0";
static void mapimg_log(const char *file, const char *function, int line,
                       const char *format, ...)
                       fc__attribute((__format__(__printf__, 4, 5)));
#define MAPIMG_LOG(format, ...)                                             \
  mapimg_log(__FILE__, __FUNCTION__, __FC_LINE__, format, ## __VA_ARGS__)
#define MAPIMG_ASSERT_RET_VAL(cond, expr)                                   \
  fc_assert_action(cond, MAPIMG_LOG(_("internal error")); return (expr))

/* == additional functions == */

static int bvplayers_count(const struct mapdef *pmapdef);
static const char *bvplayers_str(const bv_player plrbv);

/* == map images data == */
static struct {
  bool init;
  struct mapdef_list *mapdef;

  mapimg_tile_known_func mapimg_tile_known;
  mapimg_tile_terrain_func mapimg_tile_terrain;
  mapimg_tile_player_func mapimg_tile_owner;
  mapimg_tile_player_func mapimg_tile_city;
  mapimg_tile_player_func mapimg_tile_unit;
  mapimg_plrcolor_count_func mapimg_plrcolor_count;
  mapimg_plrcolor_get_func mapimg_plrcolor_get;
} mapimg = { .init = FALSE };

/*
 * ==============================================
 * map images (external functions)
 * ==============================================
 */

/************************************************************************//**
  Initialisation of the map image subsystem. The arguments are used to
  determine the map knowledge, the terrain type as well as the tile, city and
  unit owner.
****************************************************************************/
void mapimg_init(mapimg_tile_known_func mapimg_tile_known,
                 mapimg_tile_terrain_func mapimg_tile_terrain,
                 mapimg_tile_player_func mapimg_tile_owner,
                 mapimg_tile_player_func mapimg_tile_city,
                 mapimg_tile_player_func mapimg_tile_unit,
                 mapimg_plrcolor_count_func mapimg_plrcolor_count,
                 mapimg_plrcolor_get_func mapimg_plrcolor_get)
{
  if (mapimg_initialised()) {
    return;
  }

  mapimg.mapdef = mapdef_list_new();

  fc_assert_ret(mapimg_tile_known != NULL);
  mapimg.mapimg_tile_known = mapimg_tile_known;
  fc_assert_ret(mapimg_tile_terrain != NULL);
  mapimg.mapimg_tile_terrain = mapimg_tile_terrain;
  fc_assert_ret(mapimg_tile_owner != NULL);
  mapimg.mapimg_tile_owner = mapimg_tile_owner;
  fc_assert_ret(mapimg_tile_city != NULL);
  mapimg.mapimg_tile_city = mapimg_tile_city;
  fc_assert_ret(mapimg_tile_unit != NULL);
  mapimg.mapimg_tile_unit = mapimg_tile_unit;
  fc_assert_ret(mapimg_plrcolor_count != NULL);
  mapimg.mapimg_plrcolor_count = mapimg_plrcolor_count;
  fc_assert_ret(mapimg_plrcolor_get != NULL);
  mapimg.mapimg_plrcolor_get = mapimg_plrcolor_get;

  mapimg.init = TRUE;
}

/************************************************************************//**
  Reset the map image subsystem.
****************************************************************************/
void mapimg_reset(void)
{
  if (!mapimg_initialised()) {
    return;
  }

  if (mapdef_list_size(mapimg.mapdef) > 0) {
    mapdef_list_iterate(mapimg.mapdef, pmapdef) {
      mapdef_list_remove(mapimg.mapdef, pmapdef);
      mapdef_destroy(pmapdef);
    } mapdef_list_iterate_end;
  }
}

/************************************************************************//**
  Free all memory allocated by the map image subsystem.
****************************************************************************/
void mapimg_free(void)
{
  if (!mapimg_initialised()) {
    return;
  }

  mapimg_reset();
  mapdef_list_destroy(mapimg.mapdef);

  mapimg.init = FALSE;
}

/************************************************************************//**
  Return the number of map image definitions.
****************************************************************************/
int mapimg_count(void)
{
  if (!mapimg_initialised()) {
    return 0;
  }

  return mapdef_list_size(mapimg.mapdef);
}

/************************************************************************//**
  Describe the 'show' settings.
****************************************************************************/
static const char *showname_help(enum show_player showplr)
{
  switch (showplr) {
  case SHOW_NONE:     return _("no players, only terrain");
  case SHOW_EACH:     return _("one image per player");
  case SHOW_HUMAN:    return _("one image per human player");
  case SHOW_ALL:      return _("all players on a single image");
  case SHOW_PLRNAME:  return _("just the player named with 'plrname'");
  case SHOW_PLRID:    return _("just the player specified with 'plrid'");
  case SHOW_PLRBV:    return _("one image per player in 'plrbv'");
  }
  fc_assert(0);
  return "";
}

/************************************************************************//**
  Return a help string for the 'mapimg' command.
****************************************************************************/
char *mapimg_help(const char *cmdname)
{
  enum imagetool tool;
  enum show_player showplr;
  enum mapimg_layer layer;
  struct astring defaults[MAPDEF_COUNT];
  struct astring str_format = ASTRING_INIT, str_showplr = ASTRING_INIT;
  struct mapdef *pmapdef = mapdef_new(FALSE);
  static struct astring help = ASTRING_INIT;

  if (astr_len(&help) > 0) {
    /* Help text was created already. */
    return fc_strdup(astr_str(&help));
  }

  /* Possible 'format' settings (toolkit + format). */
  for (tool = imagetool_begin(); tool != imagetool_end();
       tool = imagetool_next(tool)) {
    enum imageformat format;
    const struct toolkit *toolkit = img_toolkit_get(tool);

    if (!toolkit) {
      continue;
    }

    astr_add(&str_format, " - '%s': ", imagetool_name(tool));

    const char *separator = "";
    for (format = imageformat_begin(); format != imageformat_end();
         format = imageformat_next(format)) {
      if (toolkit->formats & format) {
        astr_add(&str_format, "%s'%s'", separator, imageformat_name(format));
        separator = ", ";
      }
    }

    if (tool != imagetool_max()) {
      astr_add(&str_format, "\n");
    }
  }

  /* Possible 'show' settings. */
  for (showplr = show_player_begin(); showplr != show_player_end();
       showplr = show_player_next(showplr)) {
    char name[10];
    fc_snprintf(name, sizeof(name), "'%s'", show_player_name(showplr));
    astr_add(&str_showplr, " - %-9s %s", name, showname_help(showplr));
    if (showplr != show_player_max()) {
      astr_add(&str_showplr, "\n");
    }
  }

  /* Default values. */
  astr_init(&defaults[MAPDEF_FORMAT]);
  astr_set(&defaults[MAPDEF_FORMAT],
           "(%s|%s)", imagetool_name(pmapdef->tool),
           imageformat_name(pmapdef->format));
  astr_init(&defaults[MAPDEF_SHOW]);
  astr_set(&defaults[MAPDEF_SHOW],
           "(%s)", show_player_name(pmapdef->player.show));
  astr_init(&defaults[MAPDEF_TURNS]);
  astr_set(&defaults[MAPDEF_TURNS], "(%d)", pmapdef->turns);
  astr_init(&defaults[MAPDEF_ZOOM]);
  astr_set(&defaults[MAPDEF_ZOOM], "(%d)", pmapdef->zoom);

  astr_init(&defaults[MAPDEF_MAP]);
  astr_set(&defaults[MAPDEF_MAP], "(");
  for (layer = mapimg_layer_begin(); layer != mapimg_layer_end();
       layer = mapimg_layer_next(layer)) {
    if (pmapdef->layers[layer]) {
      astr_add(&defaults[MAPDEF_MAP], "%c", mapimg_layer_name(layer)[0]);
    }
  }
  astr_add(&defaults[MAPDEF_MAP], ")");

  /* help text */
  astr_set(&help,
    /* TRANS: This is help for a server command, so keywords like
     * "define" in the first column are server keywords that must not
     * be translated. Do not translate keywords in single quotes, but
     * strings in <angle brackets> should be translated. */
    _("This command controls the creation of map images. Supported "
      "arguments:\n"
      "  define <mapdef>  - define a map image; returns numeric <id>\n"
      "  show <id>|all    - list map image definitions or show a specific one\n"
      "  create <id>|all  - manually save image(s) for current map state\n"
      "  delete <id>|all  - delete map image definition(s)\n"
      "  colortest        - create test image(s) showing all colors\n"
      "\n"
      "Multiple definitions can be active at once. "
      "A definition <mapdef> consists of colon-separated options:\n"
      "\n"
      "option                 (default)  description\n"
      "\n"
      "format=<[tool|]format> %-10s file format\n"
      "show=<show>            %-10s which players to show\n"
      "  plrname=<name>                    player name\n"
      "  plrid=<id>                        numeric player id\n"
      "  plrbv=<bit vector>                see example; first char = id 0\n"
      "turns=<turns>          %-10s save image each <turns> turns\n"
      "                                  (0=no autosave, save with 'create')\n"
      "zoom=<zoom>            %-10s magnification factor (1-5)\n"
      "map=<map>              %-10s which map layers to draw\n"
      "\n"
      "<[tool|]format> = use image format <format>, optionally specifying "
      "toolkit <tool>. The following toolkits and formats are compiled in:\n"
      "%s\n"
      "\n"
      "<show> determines which players are represented and how many "
      "images are saved by this definition:\n"
      "%s\n"
      "\n"
      "<map> can contain one or more of the following layers:\n"
      " - 'a' show area within borders of specified players\n"
      " - 'b' show borders of specified players\n"
      " - 'c' show cities of specified players\n"
      " - 'f' show fog of war (single-player images only)\n"
      " - 'k' show only player knowledge (single-player images only)\n"
      " - 't' full display of terrain types\n"
      " - 'u' show units of specified players\n"
      "\n"
      "Examples of <mapdef>:\n"
      " 'zoom=1:map=tcub:show=all:format=ppm|ppm'\n"
      " 'zoom=2:map=tcub:show=each:format=png'\n"
      " 'zoom=1:map=tcub:show=plrname:plrname=Otto:format=gif'\n"
      " 'zoom=3:map=cu:show=plrbv:plrbv=010011:format=jpg'\n"
      " 'zoom=1:map=t:show=none:format=magick|jpg'"),
    astr_str(&defaults[MAPDEF_FORMAT]), astr_str(&defaults[MAPDEF_SHOW]),
    astr_str(&defaults[MAPDEF_TURNS]), astr_str(&defaults[MAPDEF_ZOOM]),
    astr_str(&defaults[MAPDEF_MAP]),
    astr_str(&str_format), astr_str(&str_showplr));
  
  mapdef_destroy(pmapdef);
  astr_free(&str_format);
  astr_free(&str_showplr);
  astr_free(&defaults[MAPDEF_FORMAT]);
  astr_free(&defaults[MAPDEF_SHOW]);
  astr_free(&defaults[MAPDEF_TURNS]);
  astr_free(&defaults[MAPDEF_ZOOM]);
  astr_free(&defaults[MAPDEF_MAP]);

  return fc_strdup(astr_str(&help));
}

/************************************************************************//**
  Returns the last error.
****************************************************************************/
const char *mapimg_error(void)
{
  return error_buffer;
}

/************************************************************************//**
  Define on map image.
****************************************************************************/
#define NUM_MAX_MAPARGS 10
#define NUM_MAX_MAPOPTS 2
bool mapimg_define(const char *maparg, bool check)
{
  struct mapdef *pmapdef = NULL;
  char *mapargs[NUM_MAX_MAPARGS], *mapopts[NUM_MAX_MAPOPTS];
  int nmapargs, nmapopts, i;
  bool ret = TRUE;

  MAPIMG_ASSERT_RET_VAL(mapimg_initialised(), FALSE);

  if (maparg == NULL) {
    MAPIMG_LOG(_("no map definition"));
    return FALSE;
  }

  if (strlen(maparg) > MAX_LEN_MAPARG) {
    /* too long map definition string */
    MAPIMG_LOG(_("map definition string too long (max %d characters)"),
               MAX_LEN_MAPARG);
    return FALSE;
  }

  if (mapimg_count() == MAX_NUM_MAPIMG) {
    MAPIMG_LOG(_("maximum number of map definitions reached (%d)"),
               MAX_NUM_MAPIMG);
    return FALSE;
  }

  for (i = 0; i < mapimg_count(); i++) {
    pmapdef = mapdef_list_get(mapimg.mapdef, i);
    if (0 == fc_strcasecmp(pmapdef->maparg, maparg)) {
      MAPIMG_LOG(_("duplicate of map image definition %d ('%s')"), i,
                 maparg);
      return FALSE;
    }
  }

  pmapdef = mapdef_new(FALSE);

  /* get map options */
  nmapargs = get_tokens(maparg, mapargs, NUM_MAX_MAPARGS, ":");

  for (i = 0; i < nmapargs; i++) {
    /* split map options into variable and value */
    nmapopts = get_tokens(mapargs[i], mapopts, NUM_MAX_MAPOPTS, "=");

    if (nmapopts == 2) {
      enum mapdef_arg arg = mapdef_arg_by_name(mapopts[0], strcmp);
      if (mapdef_arg_is_valid(arg)) {
        /* If ret is FALSE an error message is set by mapimg_define_arg(). */
        ret = mapimg_define_arg(pmapdef, arg, mapopts[1], check);
      } else {
        MAPIMG_LOG(_("unknown map option: '%s'"), mapargs[i]);
        ret = FALSE;
      }
    } else {
      MAPIMG_LOG(_("unknown map option: '%s'"), mapargs[i]);
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
    if (!BV_ISSET(pmapdef->args, MAPDEF_PLRNAME)) {
      MAPIMG_LOG(_("'show=%s' but no player name 'plrname'"),
                 show_player_name(SHOW_PLRNAME));
      ret = FALSE;
    }
    break;
  case SHOW_PLRID:   /* display player given by id */
    if (!BV_ISSET(pmapdef->args, MAPDEF_PLRID)) {
      MAPIMG_LOG(_("'show=%s' but no player id 'plrid'"),
                 show_player_name(SHOW_PLRID));
      ret = FALSE;
    }
    break;
  case SHOW_PLRBV:   /* display players given by bitvector */
    if (!BV_ISSET(pmapdef->args, MAPDEF_PLRBV)) {
      MAPIMG_LOG(_("'show=%s' but no player bitvector 'plrbv'"),
                 show_player_name(SHOW_PLRBV));
      ret = FALSE;
    }
    break;
  case SHOW_NONE:    /* no player on the map */
    BV_CLR_ALL(pmapdef->player.checked_plrbv);
    break;
  case SHOW_ALL:     /* show all players in one map */
    BV_SET_ALL(pmapdef->player.checked_plrbv);
    break;
  case SHOW_EACH:    /* one map for each player */
  case SHOW_HUMAN:   /* one map for each human player */
    /* A loop for each player will be called at the time the image is
     * created. */
    BV_CLR_ALL(pmapdef->player.checked_plrbv);
    break;
  }

  if (ret && !check) {
    /* save map string */
    fc_strlcpy(pmapdef->maparg, maparg, MAX_LEN_MAPARG);

    /* add map definiton */
    mapdef_list_append(mapimg.mapdef, pmapdef);
  } else {
    mapdef_destroy(pmapdef);
  }

  return ret;
}
#undef NUM_MAX_MAPARGS
#undef NUM_MAX_MAPOPTS

/************************************************************************//**
  Helper function for mapimg_define().
****************************************************************************/
#define NUM_MAX_FORMATARGS 2
static bool mapimg_define_arg(struct mapdef *pmapdef, enum mapdef_arg arg,
                              const char *val, bool check)
{
  if (BV_ISSET(pmapdef->args, arg)) {
    log_debug("Option '%s' for mapimg used more than once.",
              mapdef_arg_name(arg));
  }

  /* This argument was used. */
  BV_SET(pmapdef->args, arg);

  switch (arg) {
  case MAPDEF_FORMAT:
    /* file format */
    {
      char *formatargs[NUM_MAX_FORMATARGS];
      int nformatargs;
      enum imageformat format;
      enum imagetool tool;
      bool error = TRUE;

      /* get format options */
      nformatargs = get_tokens(val, formatargs, NUM_MAX_FORMATARGS, "|");

      if (nformatargs == 2) {
        tool = imagetool_by_name(formatargs[0], strcmp);
        format = imageformat_by_name(formatargs[1], strcmp);

        if (imageformat_is_valid(format) && imagetool_is_valid(tool)) {
          const struct toolkit *toolkit = img_toolkit_get(tool);

          if (toolkit && (toolkit->formats & format)) {
            pmapdef->tool = tool;
            pmapdef->format = format;

            error = FALSE;
          }
        }
      } else {
        /* Only one argument to format. */
        tool = imagetool_by_name(formatargs[0], strcmp);
        if (imagetool_is_valid(tool)) {
          /* toolkit defined */
          const struct toolkit *toolkit = img_toolkit_get(tool);

          if (toolkit) {
            pmapdef->tool = toolkit->tool;
            pmapdef->format = toolkit->format_default;

            error = FALSE;
          }
        } else {
          format = imageformat_by_name(formatargs[0], strcmp);
          if (imageformat_is_valid(format)) {
            /* format defined */
            img_toolkit_iterate(toolkit) {
              if ((toolkit->formats & format)) {
                pmapdef->tool = toolkit->tool;
                pmapdef->format = toolkit->format_default;

                error = FALSE;
                break;
              }
            } img_toolkit_iterate_end;
          }
        }
      }

      free_tokens(formatargs, nformatargs);

      if (error) {
        goto INVALID;
      }
    }
    break;

  case MAPDEF_MAP:
    /* map definition */
    {
      int len = strlen(val), l;
      enum mapimg_layer layer;
      bool error;

      for (layer = mapimg_layer_begin(); layer != mapimg_layer_end();
           layer = mapimg_layer_next(layer)) {
        pmapdef->layers[layer] = FALSE;
      }

      for (l = 0; l < len; l++) {
        error = TRUE;
        for (layer = mapimg_layer_begin(); layer != mapimg_layer_end();
             layer = mapimg_layer_next(layer)) {
          if (val[l] == mapimg_layer_name(layer)[0]) {
            pmapdef->layers[layer] = TRUE;
            error = FALSE;
            break;
          }
        }

        if (error) {
          goto INVALID;
        }
      }
    }
    break;

  case MAPDEF_PLRBV:
    /* player definition - bitvector */
    {
      int i;

      if (strlen(val) < MAX_NUM_PLAYER_SLOTS + 1) {
        BV_CLR_ALL(pmapdef->player.plrbv);
        for (i = 0; i < strlen(val); i++) {
          if (!strchr("01", val[i])) {
            MAPIMG_LOG(_("invalid character in bitvector: '%c' (%s)"),
                       val[i], val);
            return FALSE;
          } else if (val[i] == '1') {
            BV_SET(pmapdef->player.plrbv, i);
          }
        }
      } else {
        goto INVALID;
      }
    }
    break;

  case MAPDEF_PLRID:
    /* player definition - player id; will be checked by mapimg_isvalid()
     * which calls mapimg_checkplayers() */
    {
      int plrid;

      if (sscanf(val, "%d", &plrid) != 0) {
        if (plrid < 0 || plrid >= MAX_NUM_PLAYER_SLOTS) {
          MAPIMG_LOG(_("'plrid' should be between 0 and %d"),
                     MAX_NUM_PLAYER_SLOTS-1);
          return FALSE;
        }
        pmapdef->player.id = plrid;
      } else {
        goto INVALID;
      }
    }
    break;

  case MAPDEF_PLRNAME:
    /* player definition - player name; will be checked by mapimg_isvalid()
     * which calls mapimg_checkplayers() */
    {
      if (strlen(val) > sizeof(pmapdef->player.name)) {
        MAPIMG_LOG(_("player name too long: '%s' (max: %lu)"), val,
                   (unsigned long) sizeof(pmapdef->player.name));
        return FALSE;
      } else {
        sz_strlcpy(pmapdef->player.name, val);
      }
    }
    break;

  case MAPDEF_SHOW:
    /* player definition - basic definition */
    {
      enum show_player showplr;

      showplr = show_player_by_name(val, strcmp);
      if (show_player_is_valid(showplr)) {
        pmapdef->player.show = showplr;
      } else {
        goto INVALID;
      }
    }
    break;

  case MAPDEF_TURNS:
    /* save each <x> turns */
    {
      int turns;

      if (sscanf(val, "%d", &turns) != 0) {
        if (turns < 0 || turns > 99) {
          MAPIMG_LOG(_("'turns' should be between 0 and 99"));
          return FALSE;
        } else {
          pmapdef->turns = turns;
        }
      } else {
        goto INVALID;
      }
    }
    break;

  case MAPDEF_ZOOM:
    /* zoom factor */
    {
      int zoom;

      if (sscanf(val, "%d", &zoom) != 0) {
        if (zoom < 1 || zoom > 5) {
          MAPIMG_LOG(_("'zoom' factor should be between 1 and 5"));
          return FALSE;
        } else {
          pmapdef->zoom = zoom;
        }
      } else {
        goto INVALID;
      }
    }
    break;

  case MAPDEF_COUNT:
    fc_assert_ret_val(arg != MAPDEF_COUNT, FALSE);
    break;
  }

  return TRUE;

 INVALID:
  MAPIMG_LOG(_("invalid value for option '%s': '%s'"),
             mapdef_arg_name(arg), val);
  return FALSE;
}
#undef NUM_MAX_FORMATARGS

/************************************************************************//**
  Check if a map image definition is valid. This function is a wrapper for
  mapimg_checkplayers().
****************************************************************************/
struct mapdef *mapimg_isvalid(int id)
{
  struct mapdef *pmapdef = NULL;

  if (!mapimg_test(id)) {
    /* The error message is set in mapimg_test(). */
    return NULL;
  }

  pmapdef = mapdef_list_get(mapimg.mapdef, id);
  mapimg_checkplayers(pmapdef, TRUE);

  switch (pmapdef->status) {
  case MAPIMG_STATUS_UNKNOWN:
    MAPIMG_LOG(_("map definition not checked (game not started)"));
    return NULL;
    break;
  case MAPIMG_STATUS_ERROR:
    MAPIMG_LOG(_("map definition deactivated: %s"), pmapdef->error);
    return NULL;
    break;
  case MAPIMG_STATUS_OK:
    /* nothing */
    break;
  }

  return pmapdef;
}

/************************************************************************//**
  Return a list of all available tookits and formats for the client.
****************************************************************************/
const struct strvec *mapimg_get_format_list(void)
{
  static struct strvec *format_list = NULL;

  if (NULL == format_list) {
    enum imagetool tool;

    format_list = strvec_new();

    for (tool = imagetool_begin(); tool != imagetool_end();
         tool = imagetool_next(tool)) {
      enum imageformat format;
      const struct toolkit *toolkit = img_toolkit_get(tool);

      if (!toolkit) {
        continue;
      }

      for (format = imageformat_begin(); format != imageformat_end();
           format = imageformat_next(format)) {
        if (toolkit->formats & format) {
          char str_format[64];
          fc_snprintf(str_format, sizeof(str_format), "%s|%s",
                      imagetool_name(tool), imageformat_name(format));
          strvec_append(format_list, str_format);
        }
      }
    }
  }

  return format_list;
}

/************************************************************************//**
  Return the default value of the tookit and the image format for the client.
****************************************************************************/
const char *mapimg_get_format_default(void)
{
  static char default_format[64];

  fc_snprintf(default_format, sizeof(default_format), "%s|%s",
              imagetool_name(MAPIMG_DEFAULT_IMGTOOL),
              imageformat_name(MAPIMG_DEFAULT_IMGFORMAT));

  return default_format;
}

/************************************************************************//**
  Delete a map image definition.
****************************************************************************/
bool mapimg_delete(int id)
{
  struct mapdef *pmapdef = NULL;

  if (!mapimg_test(id)) {
    /* The error message is set in mapimg_test(). */
    return FALSE;
  }

  /* delete map definition */
  pmapdef = mapdef_list_get(mapimg.mapdef, id);
  mapdef_list_remove(mapimg.mapdef, pmapdef);

  return TRUE;
}

/************************************************************************//**
  Show a map image definition.
****************************************************************************/
bool mapimg_show(int id, char *str, size_t str_len, bool detail)
{
  struct mapdef *pmapdef = NULL;

  if (!mapimg_test(id)) {
    /* The error message is set in mapimg_test(). */
    return FALSE;
  }

  pmapdef = mapdef_list_get(mapimg.mapdef, id);

  /* Clear string ... */
  fc_assert_ret_val(str_len > 0, FALSE);
  str[0] = '\0';

  if (detail) {
    cat_snprintf(str, str_len, _("Detailed information for map image "
                                 "definition %d\n"), id);
    if (pmapdef->status == MAPIMG_STATUS_ERROR) {
      cat_snprintf(str, str_len, _("  - status:                   %s (%s)\n"),
                   mapimg_status_name(pmapdef->status), pmapdef->error);
    } else {
      cat_snprintf(str, str_len, _("  - status:                   %s\n"),
                   mapimg_status_name(pmapdef->status));
    }
    cat_snprintf(str, str_len, _("  - file name string:         %s\n"),
                 mapimg_generate_name(pmapdef));
    cat_snprintf(str, str_len, _("  - image toolkit:            %s\n"),
                 imagetool_name(pmapdef->tool));
    cat_snprintf(str, str_len, _("  - image format:             %s\n"),
                 imageformat_name(pmapdef->format));
    cat_snprintf(str, str_len, _("  - zoom factor:              %d\n"),
                 pmapdef->zoom);
    cat_snprintf(str, str_len, _("  - show area within borders: %s\n"),
                 pmapdef->layers[MAPIMG_LAYER_AREA] ? _("yes") : _("no"));
    cat_snprintf(str, str_len, _("  - show borders:             %s\n"),
                 pmapdef->layers[MAPIMG_LAYER_BORDERS] ? _("yes") : _("no"));
    cat_snprintf(str, str_len, _("  - show cities:              %s\n"),
                 pmapdef->layers[MAPIMG_LAYER_CITIES] ? _("yes") : _("no"));
    cat_snprintf(str, str_len, _("  - show fog of war:          %s\n"),
                 pmapdef->layers[MAPIMG_LAYER_FOGOFWAR] ? _("yes") : _("no"));
    cat_snprintf(str, str_len, _("  - show player knowledge:    %s\n"),
                 pmapdef->layers[MAPIMG_LAYER_KNOWLEDGE] ? _("yes") : _("no"));
    cat_snprintf(str, str_len, _("  - show terrain:             %s\n"),
                 pmapdef->layers[MAPIMG_LAYER_TERRAIN] ? _("full") :_("basic"));
    cat_snprintf(str, str_len, _("  - show units:               %s\n"),
                 pmapdef->layers[MAPIMG_LAYER_UNITS] ? _("yes") : _("no"));
    cat_snprintf(str, str_len, _("  - players included:         %s"),
                 show_player_name(pmapdef->player.show));
    switch (pmapdef->player.show) {
    case SHOW_NONE:
    case SHOW_HUMAN:
    case SHOW_EACH:
    case SHOW_ALL:
      /* nothing */
      break;
    case SHOW_PLRNAME:
      cat_snprintf(str, str_len, _("\n  - player name:              %s"),
                   pmapdef->player.name);
      break;
    case SHOW_PLRID:
      cat_snprintf(str, str_len, _("\n  - player id:                %d"),
                   pmapdef->player.id);
      break;
    case SHOW_PLRBV:
      cat_snprintf(str, str_len, _("\n  - players:                  %s"),
                   bvplayers_str(pmapdef->player.plrbv));
      break;
    }
  } else {
    char str_def[MAX_LEN_MAPDEF];
    mapimg_def2str(pmapdef, str_def, sizeof(str_def));
    if (pmapdef->status == MAPIMG_STATUS_ERROR) {
      cat_snprintf(str, str_len, "'%s' (%s: %s)", str_def,
                   mapimg_status_name(pmapdef->status), pmapdef->error);
    } else {
      cat_snprintf(str, str_len, "'%s' (%s)", str_def,
                   mapimg_status_name(pmapdef->status));
    }
  }

  return TRUE;
}

/************************************************************************//**
  Return the map image definition 'id' as a mapdef string. This function is
  a wrapper for mapimg_def2str().
****************************************************************************/
bool mapimg_id2str(int id, char *str, size_t str_len)
{
  struct mapdef *pmapdef = NULL;

  if (!mapimg_test(id)) {
    /* The error message is set in mapimg_test(). */
    return FALSE;
  }

  pmapdef = mapdef_list_get(mapimg.mapdef, id);

  return mapimg_def2str(pmapdef, str, str_len);
}

/************************************************************************//**
  Create the requested map image. The filename is created as
  <basename as used for savegames>-<mapstr>.<mapext> where <mapstr>
  contains the map definition and <mapext> the selected image extension.
  If 'force' is FALSE, the image is only created if game.info.turn is a
  multiple of the map setting turns.
****************************************************************************/
bool mapimg_create(struct mapdef *pmapdef, bool force, const char *savename,
                   const char *path)
{
  struct img *pimg;
  char mapimgfile[MAX_LEN_PATH];
  bool ret = TRUE;
#ifdef FREECIV_DEBUG
  struct timer *timer_cpu, *timer_user;
#endif

  if (map_is_empty()) {
    MAPIMG_LOG(_("map not yet created"));

    return FALSE;
  }

  mapimg_checkplayers(pmapdef, FALSE);

  if (pmapdef->status != MAPIMG_STATUS_OK) {
    MAPIMG_LOG(_("map definition not checked or error"));
    return FALSE;
  }

  /* An image should be saved if:
   * - force is set to TRUE
   * - it is the first turn
   * - turns is set to a value not zero and the current turn can be devided
   *   by this number */
  if (!force && game.info.turn != 1
      && !(pmapdef->turns != 0 && game.info.turn % pmapdef->turns == 0)) {
    return TRUE;
  }

#ifdef FREECIV_DEBUG
  timer_cpu = timer_new(TIMER_CPU, TIMER_ACTIVE);
  timer_start(timer_cpu);
  timer_user = timer_new(TIMER_USER, TIMER_ACTIVE);
  timer_start(timer_user);
#endif /* FREECIV_DEBUG */

  /* create map */
  switch (pmapdef->player.show) {
  case SHOW_PLRNAME: /* display player given by name */
  case SHOW_PLRID:   /* display player given by id */
  case SHOW_NONE:    /* no player one the map */
  case SHOW_ALL:     /* show all players in one map */
  case SHOW_PLRBV:   /* display player(s) given by bitvector */

    pimg = img_new(pmapdef, CURRENT_TOPOLOGY, wld.map.xsize, wld.map.ysize);
    img_createmap(pimg);
    if (!img_save(pimg, savename, path)) {
      ret = FALSE;
    }
    img_destroy(pimg);
    break;
  case SHOW_EACH:    /* one map for each player */
  case SHOW_HUMAN:   /* one map for each human player */
    players_iterate(pplayer) {
      if (!pplayer->is_alive || (pmapdef->player.show == SHOW_HUMAN
                                 && !is_human(pplayer))) {
        /* no map image for dead players
         * or AI players if only human players should be shown */
        continue;
      }

      BV_CLR_ALL(pmapdef->player.checked_plrbv);
      BV_SET(pmapdef->player.checked_plrbv, player_index(pplayer));

      generate_save_name(savename, mapimgfile, sizeof(mapimgfile),
                         mapimg_generate_name(pmapdef));

      pimg = img_new(pmapdef, CURRENT_TOPOLOGY, wld.map.xsize, wld.map.ysize);
      img_createmap(pimg);
      if (!img_save(pimg, mapimgfile, path)) {
        ret = FALSE;
      }
      img_destroy(pimg);

      if (!ret) {
        break;
      }
    } players_iterate_end;
    break;
  }

#ifdef FREECIV_DEBUG
  log_debug("Image generation time: %g seconds (%g apparent)",
            timer_read_seconds(timer_cpu),
            timer_read_seconds(timer_user));

  timer_destroy(timer_cpu);
  timer_destroy(timer_user);
#endif /* FREECIV_DEBUG */

  return ret;
}

/************************************************************************//**
  Create images which shows all map colors (playercolor, terrain colors). One
  image is created for each supported toolkit and image format. The filename
  will be <basename as used for savegames>-colortest-<tookit>.<format>.
****************************************************************************/
bool mapimg_colortest(const char *savename, const char *path)
{
  struct img *pimg;
  const struct rgbcolor *pcolor;
  struct mapdef *pmapdef = mapdef_new(TRUE);
  char mapimgfile[MAX_LEN_PATH];
  bv_pixel pixel;
  int i, nat_x, nat_y;
  int max_playercolor = mapimg.mapimg_plrcolor_count();
  int max_terraincolor = terrain_count();
  bool ret = TRUE;
  enum imagetool tool;

#define SIZE_X 16
#define SIZE_Y 5

  pimg = img_new(pmapdef, 0, SIZE_X + 2,
                 SIZE_Y * (max_playercolor / SIZE_X) + 2);

  pixel = pimg->pixel_tile(NULL, NULL, FALSE);

  pcolor = imgcolor_special(IMGCOLOR_OCEAN);
  for (i = 0; i < MAX(max_playercolor, max_terraincolor); i++) {
    nat_x = 1 + i % SIZE_X;
    nat_y = 1 + (i / SIZE_X) * SIZE_Y;

    img_plot(pimg, nat_x, nat_y, pcolor, pixel);
  }

  for (i = 0; i < MAX(max_playercolor, max_terraincolor); i++) {
    if (i >= max_playercolor) {
      break;
    }

    nat_x = 1 + i % SIZE_X;
    nat_y = 2 + (i / SIZE_X) * SIZE_Y;
    pcolor = mapimg.mapimg_plrcolor_get(i);

    img_plot(pimg, nat_x, nat_y, pcolor, pixel);
  }

  pcolor = imgcolor_special(IMGCOLOR_GROUND);
  for (i = 0; i < MAX(max_playercolor, max_terraincolor); i++) {
    nat_x = 1 + i % SIZE_X;
    nat_y = 3 + (i / SIZE_X) * SIZE_Y;

    img_plot(pimg, nat_x, nat_y, pcolor, pixel);
  }

  for (i = 0; i < MAX(max_playercolor, max_terraincolor); i++) {
    if (i >= max_terraincolor) {
      break;
    }

    nat_x = 1 + i % SIZE_X;
    nat_y = 4 + (i / SIZE_X) * SIZE_Y;
    pcolor = imgcolor_terrain(terrain_by_number(i));

    img_plot(pimg, nat_x, nat_y, pcolor, pixel);
  }

#undef SIZE_X
#undef SIZE_Y

  for (tool = imagetool_begin(); tool != imagetool_end();
       tool = imagetool_next(tool)) {
    enum imageformat format;
    const struct toolkit *toolkit = img_toolkit_get(tool);

    if (!toolkit) {
      continue;
    }

    /* Set the toolkit. */
    pmapdef->tool = tool;

    for (format = imageformat_begin(); format != imageformat_end();
         format = imageformat_next(format)) {
      if (toolkit->formats & format) {
        char buf[128];

        /* Set the image format. */
        pmapdef->format = format;

        fc_snprintf(buf, sizeof(buf), "colortest-%s", imagetool_name(tool));
        /* filename for color test */
        generate_save_name(savename, mapimgfile, sizeof(mapimgfile), buf);

        if (!img_save(pimg, mapimgfile, path)) {
          /* If one of the mapimg format/toolkit combination fail, return
           * FALSE, i.e. an error occurred. */
          ret = FALSE;
        }
      }
    }
  }

  img_destroy(pimg);
  mapdef_destroy(pmapdef);

  return ret;
}

/*
 * ==============================================
 * map images (internal functions)
 * ==============================================
 */

/************************************************************************//**
  Check if the map image subsustem is initialised.
****************************************************************************/
static inline bool mapimg_initialised(void)
{
  return mapimg.init;
}

/************************************************************************//**
  Check if the map image subsystem is initialised and the given ID is valid.
  In case of an error, the error message is saved via MAPIMG_LOG().
****************************************************************************/
static bool mapimg_test(int id)
{
  MAPIMG_ASSERT_RET_VAL(mapimg_initialised(), FALSE);

  if (id < 0 || id >= mapimg_count()) {
    MAPIMG_LOG(_("no map definition with id %d"), id);
    return FALSE;
  }

  return TRUE;
}

/************************************************************************//**
  Return a mapdef string for the map image definition given by 'pmapdef'.
****************************************************************************/
static bool mapimg_def2str(struct mapdef *pmapdef, char *str, size_t str_len)
{
  enum mapimg_layer layer;
  char buf[MAPIMG_LAYER_COUNT + 1];
  int i;

  if (pmapdef->status != MAPIMG_STATUS_OK) {
    MAPIMG_LOG(_("map definition not checked or error"));
    fc_strlcpy(str, pmapdef->maparg, str_len);
    return FALSE;
  }

  str[0] = '\0';
  cat_snprintf(str, str_len, "format=%s|%s:",
               imagetool_name(pmapdef->tool),
               imageformat_name(pmapdef->format));
  cat_snprintf(str, str_len, "turns=%d:", pmapdef->turns);

  i = 0;
  for (layer = mapimg_layer_begin(); layer != mapimg_layer_end();
       layer = mapimg_layer_next(layer)) {
    if (pmapdef->layers[layer]) {
      buf[i++] = mapimg_layer_name(layer)[0];
    }
  }
  buf[i++] = '\0';
  cat_snprintf(str, str_len, "map=%s:", buf);

  switch (pmapdef->player.show) {
  case SHOW_NONE:
  case SHOW_EACH:
  case SHOW_HUMAN:
  case SHOW_ALL:
    cat_snprintf(str, str_len, "show=%s:",
                 show_player_name(pmapdef->player.show));
    break;
  case SHOW_PLRBV:
    cat_snprintf(str, str_len, "show=%s:", show_player_name(SHOW_PLRBV));
    cat_snprintf(str, str_len, "plrbv=%s:",
                 bvplayers_str(pmapdef->player.plrbv));
    break;
  case SHOW_PLRNAME:
    cat_snprintf(str, str_len, "show=%s:", show_player_name(SHOW_PLRNAME));
    cat_snprintf(str, str_len, "plrname=%s:", pmapdef->player.name);
    break;
  case SHOW_PLRID:
    cat_snprintf(str, str_len, "show=%s:", show_player_name(SHOW_PLRID));
    cat_snprintf(str, str_len, "plrid=%d:", pmapdef->player.id);
    break;
  }
  cat_snprintf(str, str_len, "zoom=%d", pmapdef->zoom);

  return TRUE;
}

/************************************************************************//**
  Check the player selection. This needs to be done before _each_ image
  creationcall (see mapimg_create()) to test that the selection is still
  valid as players can be added or removed during the game.
****************************************************************************/
static bool mapimg_checkplayers(struct mapdef *pmapdef, bool recheck)
{
  struct player *pplayer;
  enum m_pre_result result;

  if (!recheck && pmapdef->status == MAPIMG_STATUS_ERROR) {
    return FALSE;
  }

  /* game started - generate / check bitvector for players */
  switch (pmapdef->player.show) {
  case SHOW_NONE:
    /* nothing */
    break;
  case SHOW_PLRBV:
    pmapdef->player.checked_plrbv = pmapdef->player.plrbv;
    break;
  case SHOW_EACH:
  case SHOW_HUMAN:
    /* A map for each player will be created in a loop. */
    BV_CLR_ALL(pmapdef->player.checked_plrbv);
    break;
  case SHOW_ALL:
    BV_SET_ALL(pmapdef->player.checked_plrbv);
    break;
  case SHOW_PLRNAME:
    /* display players as requested in the map definition */
    BV_CLR_ALL(pmapdef->player.checked_plrbv);
    pplayer = player_by_name_prefix(pmapdef->player.name, &result);

    if (pplayer != NULL) {
      BV_SET(pmapdef->player.checked_plrbv, player_index(pplayer));
    } else {
      pmapdef->status = MAPIMG_STATUS_ERROR;
      /* Save the error message in map definition. */
      fc_snprintf(pmapdef->error, sizeof(pmapdef->error),
                  _("unknown player name: '%s'"), pmapdef->player.name);
      MAPIMG_LOG("%s", pmapdef->error);
      return FALSE;
    }
    break;
  case SHOW_PLRID:
    /* display players as requested in the map definition */
    BV_CLR_ALL(pmapdef->player.checked_plrbv);
    pplayer = player_by_number(pmapdef->player.id);

    if (pplayer != NULL) {
      BV_SET(pmapdef->player.checked_plrbv, player_index(pplayer));
    } else {
      pmapdef->status = MAPIMG_STATUS_ERROR;
      /* Save the error message in map definition. */
      fc_snprintf(pmapdef->error, sizeof(pmapdef->error),
                  _("invalid player id: %d"), pmapdef->player.id);
      MAPIMG_LOG("%s", pmapdef->error);
      return FALSE;
    }
    break;
  }

  pmapdef->status = MAPIMG_STATUS_OK;

  return TRUE;
}

/************************************************************************//**
  Edit the error_buffer.
  Strings should not have a terminating period.
****************************************************************************/
static void mapimg_log(const char *file, const char *function, int line,
                       const char *format, ...)
{
  va_list args;

  va_start(args, format);
  fc_vsnprintf(error_buffer, sizeof(error_buffer), format, args);
  va_end(args);

#ifdef FREECIV_DEBUG
  log_debug("In %s() [%s:%d]: %s", function, file, line, error_buffer);
#endif
}

/************************************************************************//**
  Generate an identifier for a map image.

  M<map options>Z<zoom factor>P<players>

  <map options>    map options
  <zoom factor>    zoom factor
  <players>        player ID or vector of size MAX_NUM_PLAYER_SLOTS [0/1]

  For the player bitvector all MAX_NUM_PLAYER_SLOTS values are used due to
  the possibility of additional players during the game (civil war,
  barbarians).
****************************************************************************/
static char *mapimg_generate_name(struct mapdef *pmapdef)
{
  static char mapstr[256];
  char str_show[MAX_NUM_PLAYER_SLOTS + 1];
  enum mapimg_layer layer;
  int i, count = 0, plr_id = -1;

  switch (pmapdef->player.show) {
  case SHOW_NONE:
    /* no player on the map */
    sz_strlcpy(str_show, "none");
    break;
  case SHOW_ALL:
    /* show all players in one map */
    sz_strlcpy(str_show, "all");
    break;
  case SHOW_PLRBV:
  case SHOW_PLRNAME:
  case SHOW_PLRID:
  case SHOW_HUMAN:
  case SHOW_EACH:
    /* one map for each selected player; iterate over all possible player ids
     * to generate unique strings even if civil wars occur */
    for (i = 0; i < MAX_NUM_PLAYER_SLOTS; i++) {
      str_show[i] = BV_ISSET(pmapdef->player.checked_plrbv, i) ? '1' : '0';
      if (BV_ISSET(pmapdef->player.checked_plrbv, i)) {
        count++;
        plr_id = i;
      }
    }
    str_show[MAX_NUM_PLAYER_SLOTS] = '\0';

    /* Return the option name if no player is selected or the player id if
     * there is only one player selected. */
    if (count == 0) {
      fc_snprintf(str_show, sizeof(str_show), "---%s",
                  show_player_name(pmapdef->player.show));
    } else if (count == 1 && plr_id != -1) {
      fc_snprintf(str_show, sizeof(str_show), "%03d%s", plr_id,
                  show_player_name(pmapdef->player.show));
    }
    break;
  }

  fc_snprintf(mapstr, sizeof(mapstr), "M");
  for (layer = mapimg_layer_begin(); layer != mapimg_layer_end();
       layer = mapimg_layer_next(layer)) {
    if (pmapdef->layers[layer]) {
      cat_snprintf(mapstr, sizeof(mapstr), "%s", mapimg_layer_name(layer));
    } else {
      cat_snprintf(mapstr, sizeof(mapstr), "-");
    }
  }
  cat_snprintf(mapstr, sizeof(mapstr), "Z%dP%s", pmapdef->zoom, str_show);

  return mapstr;
}

/*
 * ==============================================
 * map definitions (internal functions)
 * ==============================================
 */

/************************************************************************//**
  Create a new map image definition with default values.
****************************************************************************/
static struct mapdef *mapdef_new(bool colortest)
{
  struct mapdef *pmapdef;

  pmapdef = fc_malloc(sizeof(*pmapdef));

  /* default values */
  pmapdef->maparg[0] = '\0';
  pmapdef->error[0] = '\0';
  pmapdef->status = MAPIMG_STATUS_UNKNOWN;
  pmapdef->format = MAPIMG_DEFAULT_IMGFORMAT;
  pmapdef->tool = MAPIMG_DEFAULT_IMGTOOL;
  pmapdef->zoom = 2;
  pmapdef->turns = 1;
  pmapdef->layers[MAPIMG_LAYER_TERRAIN] = FALSE;
  pmapdef->layers[MAPIMG_LAYER_CITIES] = TRUE;
  pmapdef->layers[MAPIMG_LAYER_UNITS] = TRUE;
  pmapdef->layers[MAPIMG_LAYER_BORDERS] = TRUE;
  pmapdef->layers[MAPIMG_LAYER_FOGOFWAR] = FALSE;
  pmapdef->layers[MAPIMG_LAYER_KNOWLEDGE] = TRUE;
  pmapdef->layers[MAPIMG_LAYER_AREA] = FALSE;
  pmapdef->player.show = SHOW_ALL;
  /* The union is not set at this point (player.id, player.name and
   * player.plrbv). */
  BV_CLR_ALL(pmapdef->player.checked_plrbv);
  BV_CLR_ALL(pmapdef->args);
  pmapdef->colortest = colortest;

  return pmapdef;
}

/************************************************************************//**
  Destroy a map image definition.
****************************************************************************/
static void mapdef_destroy(struct mapdef *pmapdef)
{
  if (pmapdef == NULL) {
    return;
  }

  free(pmapdef);
}

/*
 * ==============================================
 * images (internal functions)
 * ==============================================
 */

/************************************************************************//**
  Return the definition of the requested toolkit (or NULL).
****************************************************************************/
static const struct toolkit *img_toolkit_get(enum imagetool tool)
{
  img_toolkit_iterate(toolkit) {
    if (toolkit->tool == tool) {
      return toolkit;
    }
  } img_toolkit_iterate_end;

  return NULL;
}

/************************************************************************//**
  Create a new image.
****************************************************************************/
static struct img *img_new(struct mapdef *mapdef, int topo, int xsize, int ysize)
{
  struct img *pimg;

  pimg = fc_malloc(sizeof(*pimg));

  pimg->def = mapdef;
  pimg->turn = game.info.turn;
  fc_snprintf(pimg->title, sizeof(pimg->title),
              _("Turn: %4d - Year: %10s"), game.info.turn,
              calendar_text());

  pimg->mapsize.x = xsize; /* x size of the map */
  pimg->mapsize.y = ysize; /* y size of the map */

  pimg->imgsize.x = 0; /* x size of the map image */
  pimg->imgsize.y = 0; /* y size of the map image */

  if (topo_has_flag(topo, TF_HEX)) {
    /* additional space for hex maps */
    pimg->imgsize.x += TILE_SIZE / 2;
    pimg->imgsize.y += TILE_SIZE / 2;

    if (topo_has_flag(topo, TF_ISO)) {
      /* iso-hex */
      pimg->imgsize.x += (pimg->mapsize.x + pimg->mapsize.y / 2)
                         * TILE_SIZE;
      pimg->imgsize.y += (pimg->mapsize.x + pimg->mapsize.y / 2)
                         * TILE_SIZE;

      /* magic for isohexa: change size if wrapping in only one direction */
      if ((topo_has_flag(topo, TF_WRAPX) && !topo_has_flag(topo, TF_WRAPY))
          || (!topo_has_flag(topo, TF_WRAPX) && topo_has_flag(topo, TF_WRAPY))) {
        pimg->imgsize.y += (pimg->mapsize.x - pimg->mapsize.y / 2) / 2
                           * TILE_SIZE;
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
      pimg->imgsize.x += pimg->mapsize.x * TILE_SIZE;
      pimg->imgsize.y += pimg->mapsize.y * TILE_SIZE;

      pimg->tileshape = &tile_hexa;

      pimg->pixel_tile = pixel_tile_hexa;
      pimg->pixel_city = pixel_city_hexa;
      pimg->pixel_unit = pixel_unit_hexa;
      pimg->pixel_fogofwar = pixel_fogofwar_hexa;
      pimg->pixel_border = pixel_border_hexa;

      pimg->base_coor = base_coor_hexa;
    }
  } else {
    if (topo_has_flag(topo, TF_ISO)) {
      /* isometric rectangular */
      pimg->imgsize.x += (pimg->mapsize.x + pimg->mapsize.y / 2) * TILE_SIZE;
      pimg->imgsize.y += (pimg->mapsize.x + pimg->mapsize.y / 2) * TILE_SIZE;
    } else {
      /* rectangular */
      pimg->imgsize.x += pimg->mapsize.x * TILE_SIZE;
      pimg->imgsize.y += pimg->mapsize.y * TILE_SIZE;
    }

    pimg->tileshape = &tile_rect;

    pimg->pixel_tile = pixel_tile_rect;
    pimg->pixel_city = pixel_city_rect;
    pimg->pixel_unit = pixel_unit_rect;
    pimg->pixel_fogofwar = pixel_fogofwar_rect;
    pimg->pixel_border = pixel_border_rect;

    pimg->base_coor = base_coor_rect;
  }

  /* Here the map image is saved as an array of RGB color values. */
  pimg->map = fc_calloc(pimg->imgsize.x * pimg->imgsize.y,
                        sizeof(*pimg->map));
  /* Initialise map. */
  memset(pimg->map, 0, pimg->imgsize.x * pimg->imgsize.y);

  return pimg;
}

/************************************************************************//**
  Destroy a image.
****************************************************************************/
static void img_destroy(struct img *pimg)
{
  if (pimg != NULL) {
    /* do not free pimg->def */
    free(pimg->map);
    free(pimg);
  }
}

/************************************************************************//**
  Set the color of one pixel.
****************************************************************************/
static inline void img_set_pixel(struct img *pimg, const int mindex,
                                 const struct rgbcolor *pcolor)
{
  if (mindex < 0 || mindex > pimg->imgsize.x * pimg->imgsize.y) {
    log_error("invalid index: 0 <= %d < %d", mindex,
              pimg->imgsize.x * pimg->imgsize.y);
    return;
  }

  pimg->map[mindex] = pcolor;
}

/************************************************************************//**
  Get the index for an (x,y) image coordinate.
****************************************************************************/
static inline int img_index(const int x, const int y,
                            const struct img *pimg)
{
  fc_assert_ret_val(x >= 0 && x < pimg->imgsize.x, -1);
  fc_assert_ret_val(y >= 0 && y < pimg->imgsize.y, -1);

  return pimg->imgsize.x * y + x;
}

/************************************************************************//**
  Plot one tile at (x,y). Only the pixel of the tile set within 'pixel'
  are ploted.
****************************************************************************/
static void img_plot(struct img *pimg, int x, int y,
                     const struct rgbcolor *pcolor, const bv_pixel pixel)
{
  int base_x, base_y, i, mindex;

  if (!BV_ISSET_ANY(pixel)) {
    return;
  }

  pimg->base_coor(pimg, &base_x, &base_y, x, y);

  for (i = 0; i < NUM_PIXEL; i++) {
    if (BV_ISSET(pixel, i)) {
      mindex = img_index(base_x + pimg->tileshape->x[i],
                         base_y + pimg->tileshape->y[i], pimg);
      img_set_pixel(pimg, mindex, pcolor);
    }
  }
}

/************************************************************************//**
  Plot one tile. Only the pixel of the tile set within 'pixel' are ploted.
****************************************************************************/
static void img_plot_tile(struct img *pimg, const struct tile *ptile,
                          const struct rgbcolor *pcolor, const bv_pixel pixel)
{
  int x, y;

  index_to_map_pos(&x, &y, tile_index(ptile));

  img_plot(pimg, x, y, pcolor, pixel);
}

/************************************************************************//**
  Save an image as ppm file.
****************************************************************************/
static bool img_save(const struct img *pimg, const char *mapimgfile,
                     const char *path)
{
  enum imagetool tool = pimg->def->tool;
  const struct toolkit *toolkit = img_toolkit_get(tool);
  char tmpname[600];

  if (!toolkit) {
    MAPIMG_LOG(_("toolkit not defined"));
    return FALSE;
  }

  if (!path_is_absolute(mapimgfile) && path != NULL) {
    make_dir(path);

    sz_strlcpy(tmpname, path);
    if (tmpname[0] != '\0') {
      sz_strlcat(tmpname, "/");
    }
  } else {
    tmpname[0] = '\0';
  }

  sz_strlcat(tmpname, mapimgfile);

  MAPIMG_ASSERT_RET_VAL(toolkit->img_save, FALSE);

  return toolkit->img_save(pimg, tmpname);
}

/************************************************************************//**
  Save an image using magickwand as toolkit. This allows different file
  formats.

  Image structure:

  [             0]
                    border
  [+IMG_BORDER_HEIGHT]
                    title
  [+  IMG_TEXT_HEIGHT]
                    space (only if count(displayed players) > 0)
  [+IMG_SPACER_HEIGHT]
                    player line (only if count(displayed players) > 0)
  [+  IMG_LINE_HEIGHT]
                    space
  [+IMG_SPACER_HEIGHT]
                    map
  [+   map_height]
                    border
  [+IMG_BORDER_HEIGHT]

****************************************************************************/
#ifdef HAVE_MAPIMG_MAGICKWAND
#define SET_COLOR(str, pcolor)                                              \
  fc_snprintf(str, sizeof(str), "rgb(%d,%d,%d)",                            \
              pcolor->r, pcolor->g, pcolor->b);
static bool img_save_magickwand(const struct img *pimg,
                                const char *mapimgfile)
{
  const struct rgbcolor *pcolor = NULL;
  struct player *pplr_now = NULL, *pplr_only = NULL;
  bool ret = TRUE;
  char imagefile[MAX_LEN_PATH];
  char str_color[32], comment[2048] = "", title[258];
  magickwand_size_t img_width, img_height, map_width, map_height;
  int x, y, xxx, yyy, row, i, mindex, plrwidth, plroffset, textoffset;
  bool withplr = BV_ISSET_ANY(pimg->def->player.checked_plrbv);

  if (!img_filename(mapimgfile, pimg->def->format, imagefile,
                    sizeof(imagefile))) {
    MAPIMG_LOG(_("error generating the file name"));
    return FALSE;
  }

  MagickWand *mw;
  PixelIterator *imw;
  PixelWand **pmw, *pw;
  DrawingWand *dw;

  MagickWandGenesis();

  mw = NewMagickWand();
  dw = NewDrawingWand();
  pw = NewPixelWand();

  map_width = pimg->imgsize.x * pimg->def->zoom;
  map_height = pimg->imgsize.y * pimg->def->zoom;

  img_width = map_width + 2 * IMG_BORDER_WIDTH;
  img_height = map_height + 2 * IMG_BORDER_HEIGHT + IMG_TEXT_HEIGHT
               + IMG_SPACER_HEIGHT + (withplr ? 2 * IMG_SPACER_HEIGHT : 0);

  fc_snprintf(title, sizeof(title), "%s (%s)", pimg->title, mapimgfile);

  SET_COLOR(str_color, imgcolor_special(IMGCOLOR_BACKGROUND));
  PixelSetColor(pw, str_color);
  MagickNewImage(mw, img_width, img_height, pw);

  textoffset = 0;
  if (withplr) {
    if (bvplayers_count(pimg->def) == 1) {
      /* only one player */
      for (i = 0; i < player_slot_count(); i++) {
        if (BV_ISSET(pimg->def->player.checked_plrbv, i)) {
          pplr_only = player_by_number(i);
          break;
        }
      }
    }

    if (pplr_only) {
      magickwand_size_t plr_color_square = IMG_TEXT_HEIGHT;

      textoffset += IMG_TEXT_HEIGHT + IMG_BORDER_HEIGHT;

      pcolor = imgcolor_player(player_index(pplr_only));
      SET_COLOR(str_color, pcolor);

      /* Show the color of the selected player. */
      imw = NewPixelRegionIterator(mw, IMG_BORDER_WIDTH, IMG_BORDER_HEIGHT,
                                   IMG_TEXT_HEIGHT, IMG_TEXT_HEIGHT);
      /* y coordinate */
      for (y = 0; y < IMG_TEXT_HEIGHT; y++) {
        pmw = PixelGetNextIteratorRow(imw, &plr_color_square);
        /* x coordinate */
        for (x = 0; x < IMG_TEXT_HEIGHT; x++) {
          PixelSetColor(pmw[x], str_color);
        }
        PixelSyncIterator(imw);
      }
      DestroyPixelIterator(imw);
    }

    /* Show a line displaying the colors of alive players */
    plrwidth = map_width / MIN(map_width, player_count());
    plroffset = (map_width - MIN(map_width, plrwidth * player_count())) / 2;

    imw = NewPixelRegionIterator(mw, IMG_BORDER_WIDTH,
                                 IMG_BORDER_HEIGHT + IMG_TEXT_HEIGHT
                                 + IMG_SPACER_HEIGHT, map_width,
                                 IMG_LINE_HEIGHT);
    /* y coordinate */
    for (y = 0; y < IMG_LINE_HEIGHT; y++) {
      pmw = PixelGetNextIteratorRow(imw, &map_width);

      /* x coordinate */
      for (x = plroffset; x < map_width; x++) {
        i = (x - plroffset) / plrwidth;
        pplr_now = player_by_number(i);

        if (i > player_count() || pplr_now == NULL || !pplr_now->is_alive) {
          continue;
        }

        if (BV_ISSET(pimg->def->player.checked_plrbv, i)) {
          /* The selected player is alive - display it. */
          pcolor = imgcolor_player(i);
          SET_COLOR(str_color, pcolor);
          PixelSetColor(pmw[x], str_color);
        } else if (pplr_only != NULL) {
          /* Display the state between pplr_only and pplr_now:
           *  - if allied:
           *      - show each second pixel
           *  - if pplr_now does shares map with pplr_onlyus:
           *      - show every other line of pixels
           * This results in the following patterns (# = color):
           *   ######      # # #       ######
           *                # # #       # # #
           *   ######      # # #       ######
           *                # # #       # # #
           *   shared      allied      shared vision
           *   vision                   + allied */
          if ((pplayers_allied(pplr_now, pplr_only) && (x + y) % 2 == 0)
              || (y % 2 == 0 && gives_shared_vision(pplr_now, pplr_only))) {
            pcolor = imgcolor_player(i);
            SET_COLOR(str_color, pcolor);
            PixelSetColor(pmw[x], str_color);
          }
        }
      }
      PixelSyncIterator(imw);
    }
    DestroyPixelIterator(imw);
  }

  /* Display the image name. */
  SET_COLOR(str_color, imgcolor_special(IMGCOLOR_TEXT));
  PixelSetColor(pw, str_color);
  DrawSetFillColor(dw, pw);
  DrawSetFont(dw, "Times-New-Roman");
  DrawSetFontSize(dw, IMG_TEXT_HEIGHT);
  DrawAnnotation(dw, IMG_BORDER_WIDTH + textoffset,
                 IMG_TEXT_HEIGHT + IMG_BORDER_HEIGHT,
                 (unsigned char *)title);
  MagickDrawImage(mw, dw);

  /* Display the map. */
  imw = NewPixelRegionIterator(mw, IMG_BORDER_WIDTH,
                               IMG_BORDER_HEIGHT + IMG_TEXT_HEIGHT
                               + IMG_SPACER_HEIGHT
                               + (withplr ? (IMG_LINE_HEIGHT + IMG_SPACER_HEIGHT)
                                          : 0), map_width, map_height);
  /* y coordinate */
  for (y = 0; y < pimg->imgsize.y; y++) {
    /* zoom for y */
    for (yyy = 0; yyy < pimg->def->zoom; yyy++) {

      pmw = PixelGetNextIteratorRow(imw, &map_width);

      /* x coordinate */
      for (x = 0; x < pimg->imgsize.x; x++) {
        mindex = img_index(x, y, pimg);
        pcolor = pimg->map[mindex];

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

  cat_snprintf(comment, sizeof(comment), "map definition: %s\n",
               pimg->def->maparg);
  if (BV_ISSET_ANY(pimg->def->player.checked_plrbv)) {
    players_iterate(pplayer) {
      if (!BV_ISSET(pimg->def->player.checked_plrbv, player_index(pplayer))) {
        continue;
      }

      pcolor = imgcolor_player(player_index(pplayer));
      cat_snprintf(comment, sizeof(comment), "%s\n", img_playerstr(pplayer));
    } players_iterate_end;
  }
  MagickCommentImage(mw, comment);

  if (!MagickWriteImage(mw, imagefile)) {
    MAPIMG_LOG(_("error saving map image '%s'"), imagefile);
    ret = FALSE;
  } else {
    log_verbose("Map image saved as '%s'.", imagefile);
  }

  DestroyDrawingWand(dw);
  DestroyPixelWand(pw);
  DestroyMagickWand(mw);

  MagickWandTerminus();

  return ret;
}
#undef SET_COLOR
#endif /* HAVE_MAPIMG_MAGICKWAND */

/************************************************************************//**
  Save an image as ppm file (toolkit: ppm).
****************************************************************************/
static bool img_save_ppm(const struct img *pimg, const char *mapimgfile)
{
  char ppmname[MAX_LEN_PATH];
  FILE *fp;
  int x, y, xxx, yyy, mindex;
  const struct rgbcolor *pcolor;

  if (pimg->def->format != IMGFORMAT_PPM) {
    MAPIMG_LOG(_("the ppm toolkit can only create images in the ppm "
                 "format"));
    return FALSE;
  }

  if (!img_filename(mapimgfile, IMGFORMAT_PPM, ppmname, sizeof(ppmname))) {
    MAPIMG_LOG(_("error generating the file name"));
    return FALSE;
  }

  fp = fopen(ppmname, "w");
  if (!fp) {
    MAPIMG_LOG(_("could not open file: %s"), ppmname);
    return FALSE;
  }

  fprintf(fp, "P3\n");
  fprintf(fp, "# version:2\n");
  fprintf(fp, "# map definition: %s\n", pimg->def->maparg);

  if (pimg->def->colortest) {
    fprintf(fp, "# color test\n");
  } else if (BV_ISSET_ANY(pimg->def->player.checked_plrbv)) {
    players_iterate(pplayer) {
      if (!BV_ISSET(pimg->def->player.checked_plrbv, player_index(pplayer))) {
        continue;
      }

      pcolor = imgcolor_player(player_index(pplayer));
      fprintf(fp, "# %s\n", img_playerstr(pplayer));
    } players_iterate_end;
  } else {
    fprintf(fp, "# no players\n");
  }

  fprintf(fp, "%d %d\n", pimg->imgsize.x * pimg->def->zoom,
          pimg->imgsize.y * pimg->def->zoom);
  fprintf(fp, "255\n");

  /* y coordinate */
  for (y = 0; y < pimg->imgsize.y; y++) {
    /* zoom for y */
    for (yyy = 0; yyy < pimg->def->zoom; yyy++) {
      /* x coordinate */
      for (x = 0; x < pimg->imgsize.x; x++) {
        mindex = img_index(x, y, pimg);
        pcolor = pimg->map[mindex];

        /* zoom for x */
        for (xxx = 0; xxx < pimg->def->zoom; xxx++) {
          if (pcolor == NULL) {
            pcolor = imgcolor_special(IMGCOLOR_BACKGROUND);
          }
          fprintf(fp, "%d %d %d\n", pcolor->r, pcolor->g, pcolor->b);
        }
      }
    }
  }

  log_verbose("Map image saved as '%s'.", ppmname);
  fclose(fp);

  return TRUE;
}

/************************************************************************//**
  Generate the final filename.
****************************************************************************/
static bool img_filename(const char *mapimgfile, enum imageformat format,
                         char *filename, size_t filename_len)
{
  fc_assert_ret_val(imageformat_is_valid(format) , FALSE);

  fc_snprintf(filename, filename_len, "%s.map.%s", mapimgfile,
              imageformat_name(format));

  return TRUE;
}

/************************************************************************//**
  Return a definition string for the player.
****************************************************************************/
static const char *img_playerstr(const struct player *pplayer)
{
  static char buf[512];
  const struct rgbcolor *pcolor = imgcolor_player(player_index(pplayer));

  fc_snprintf(buf, sizeof(buf),
              "playerno:%d:color:(%3d, %3d, %3d):name:\"%s\"",
              player_number(pplayer), pcolor->r, pcolor->g, pcolor->b,
              player_name(pplayer));

  return buf;
}

/************************************************************************//**
  Create the map considering the options (terrain, player(s), cities,
  units, borders, known, fogofwar, ...).
****************************************************************************/
static void img_createmap(struct img *pimg)
{
  const struct rgbcolor *pcolor;
  bv_pixel pixel;
  int player_id;
  struct player *pplayer = NULL;
  struct player *plr_tile = NULL, *plr_city = NULL, *plr_unit = NULL;
  enum known_type tile_knowledge = TILE_UNKNOWN;
  struct terrain *pterrain = NULL;
  bool plr_knowledge = pimg->def->layers[MAPIMG_LAYER_KNOWLEDGE];

  whole_map_iterate(&(wld.map), ptile) {
    if (bvplayers_count(pimg->def) == 1) {
      /* only one player; get player id for 'known' and 'fogofwar' */
      players_iterate(aplayer) {
        if (BV_ISSET(pimg->def->player.checked_plrbv,
                     player_index(aplayer))) {
          pplayer = aplayer;
          tile_knowledge = mapimg.mapimg_tile_known(ptile, pplayer,
                                                    plr_knowledge);
          break;
        }
      } players_iterate_end;
    }

    /* known tiles */
    if (plr_knowledge && pplayer != NULL && tile_knowledge == TILE_UNKNOWN) {
      /* plot nothing iff tile is not known */
      continue;
    }

    /* terrain */
    pterrain = mapimg.mapimg_tile_terrain(ptile, pplayer, plr_knowledge);
    if (pimg->def->layers[MAPIMG_LAYER_TERRAIN]) {
      /* full terrain */
      pixel = pimg->pixel_tile(ptile, pplayer, plr_knowledge);
      pcolor = imgcolor_terrain(pterrain);
      img_plot_tile(pimg, ptile, pcolor, pixel);
    } else {
      /* basic terrain */
      pixel = pimg->pixel_tile(ptile, pplayer, plr_knowledge);
      if (is_ocean(pterrain)) {
        img_plot_tile(pimg, ptile, imgcolor_special(IMGCOLOR_OCEAN), pixel);
      } else {
        img_plot_tile(pimg, ptile, imgcolor_special(IMGCOLOR_GROUND), pixel);
      }
    }

    /* (land) area within borders and borders */
    plr_tile = mapimg.mapimg_tile_owner(ptile, pplayer, plr_knowledge);
    if (game.info.borders > 0 && NULL != plr_tile) {
      player_id = player_index(plr_tile);
      if (pimg->def->layers[MAPIMG_LAYER_AREA] && !is_ocean(pterrain)
          && BV_ISSET(pimg->def->player.checked_plrbv, player_id)) {
        /* the tile is land and inside the players borders */
        pixel = pimg->pixel_tile(ptile, pplayer, plr_knowledge);
        pcolor = imgcolor_player(player_id);
        img_plot_tile(pimg, ptile, pcolor, pixel);
      } else if (pimg->def->layers[MAPIMG_LAYER_BORDERS]
                 && (BV_ISSET(pimg->def->player.checked_plrbv, player_id)
                     || (plr_knowledge && pplayer != NULL))) {
        /* plot borders if player is selected or view range of the one
         * displayed player */
        pixel = pimg->pixel_border(ptile, pplayer, plr_knowledge);
        pcolor = imgcolor_player(player_id);
        img_plot_tile(pimg, ptile, pcolor, pixel);
      }
    }

    /* cities and units */
    plr_city = mapimg.mapimg_tile_city(ptile, pplayer, plr_knowledge);
    plr_unit = mapimg.mapimg_tile_unit(ptile, pplayer, plr_knowledge);
    if (pimg->def->layers[MAPIMG_LAYER_CITIES] && plr_city) {
      player_id = player_index(plr_city);
      if (BV_ISSET(pimg->def->player.checked_plrbv, player_id)
          || (plr_knowledge && pplayer != NULL)) {
        /* plot cities if player is selected or view range of the one
         * displayed player */
        pixel = pimg->pixel_city(ptile, pplayer, plr_knowledge);
        pcolor = imgcolor_player(player_id);
        img_plot_tile(pimg, ptile, pcolor, pixel);
      }
    } else if (pimg->def->layers[MAPIMG_LAYER_UNITS] && plr_unit) {
      player_id = player_index(plr_unit);
      if (BV_ISSET(pimg->def->player.checked_plrbv, player_id)
          || (plr_knowledge && pplayer != NULL)) {
        /* plot units if player is selected or view range of the one
         * displayed player */
        pixel = pimg->pixel_unit(ptile, pplayer, plr_knowledge);
        pcolor = imgcolor_player(player_id);
        img_plot_tile(pimg, ptile, pcolor, pixel);
      }
    }

    /* fogofwar; if only 1 player is plotted */
    if (game.info.fogofwar && pimg->def->layers[MAPIMG_LAYER_FOGOFWAR]
        && pplayer != NULL
        && tile_knowledge == TILE_KNOWN_UNSEEN) {
      pixel = pimg->pixel_fogofwar(ptile, pplayer, plr_knowledge);
      pcolor = NULL;
      img_plot_tile(pimg, ptile, pcolor, pixel);
    }
  } whole_map_iterate_end;
}

/*
 * ==============================================
 * topology (internal functions)
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

/************************************************************************//**
   0  1  2  3  3  5
   6  7  8  9 10 11
  12 13 14 15 16 17
  18 19 20 21 22 23
  24 25 26 27 28 29
  30 31 32 33 34 35
****************************************************************************/
static bv_pixel pixel_tile_rect(const struct tile *ptile,
                                const struct player *pplayer,
                                bool knowledge)
{
  bv_pixel pixel;

  BV_SET_ALL(pixel);

  return pixel;
}

/************************************************************************//**
  -- -- -- -- -- --
  --  7  8  9 10 --
  -- 13 14 15 16 --
  -- 19 20 21 22 --
  -- 25 26 27 28 --
  -- -- -- -- -- --
****************************************************************************/
static bv_pixel pixel_city_rect(const struct tile *ptile,
                                const struct player *pplayer,
                                bool knowledge)
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

/************************************************************************//**
  -- -- -- -- -- --
  -- -- -- -- -- --
  -- -- 14 15 -- --
  -- -- 20 21 -- --
  -- -- -- -- -- --
  -- -- -- -- -- --
****************************************************************************/
static bv_pixel pixel_unit_rect(const struct tile *ptile,
                                const struct player *pplayer,
                                bool knowledge)
{
  bv_pixel pixel;

  BV_CLR_ALL(pixel);
  BV_SET(pixel, 14);
  BV_SET(pixel, 15);
  BV_SET(pixel, 20);
  BV_SET(pixel, 21);

  return pixel;
}

/************************************************************************//**
   0 --  2 --  4 --
  --  7 --  9 -- 11
  12 -- 14 -- 16 --
  -- 19 -- 21 -- 23
  24 -- 26 -- 28 --
  -- 31 -- 33 -- 35
****************************************************************************/
static bv_pixel pixel_fogofwar_rect(const struct tile *ptile,
                                    const struct player *pplayer,
                                    bool knowledge)
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

/************************************************************************//**
             [N]

       0  1  2  3  3  5
       6 -- -- -- -- 11
  [W] 12 -- -- -- -- 17 [E]
      18 -- -- -- -- 23
      24 -- -- -- -- 29
      30 31 32 33 34 35

             [S]
****************************************************************************/
static bv_pixel pixel_border_rect(const struct tile *ptile,
                                  const struct player *pplayer,
                                  bool knowledge)
{
  bv_pixel pixel;
  struct tile *pnext;
  struct player *owner;

  BV_CLR_ALL(pixel);

  fc_assert_ret_val(ptile != NULL, pixel);

  if (NULL == ptile) {
    /* no tile */
    return pixel;
  }

  owner = mapimg.mapimg_tile_owner(ptile, pplayer, knowledge);
  if (NULL == owner) {
    /* no border */
    return pixel;
  }

  pnext = mapstep(&(wld.map), ptile, DIR8_NORTH);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 0);
    BV_SET(pixel, 1);
    BV_SET(pixel, 2);
    BV_SET(pixel, 3);
    BV_SET(pixel, 4);
    BV_SET(pixel, 5);
  }

  pnext = mapstep(&(wld.map), ptile, DIR8_EAST);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 5);
    BV_SET(pixel, 11);
    BV_SET(pixel, 17);
    BV_SET(pixel, 23);
    BV_SET(pixel, 29);
    BV_SET(pixel, 35);
  }

  pnext = mapstep(&(wld.map), ptile, DIR8_SOUTH);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 30);
    BV_SET(pixel, 31);
    BV_SET(pixel, 32);
    BV_SET(pixel, 33);
    BV_SET(pixel, 34);
    BV_SET(pixel, 35);
  }

  pnext = mapstep(&(wld.map), ptile, DIR8_WEST);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 0);
    BV_SET(pixel, 6);
    BV_SET(pixel, 12);
    BV_SET(pixel, 18);
    BV_SET(pixel, 24);
    BV_SET(pixel, 30);
  }

  return pixel;
}

/************************************************************************//**
  Base coordinates for the tiles on a (isometric) rectange topology,
****************************************************************************/
static void base_coor_rect(struct img *pimg, int *base_x, int *base_y,
                           int x, int y)
{
  *base_x = x * TILE_SIZE;
  *base_y = y * TILE_SIZE;
}

/************************************************************************//**
         0  1
      2  3  4  5
   6  7  8  9 10 11
  12 13 14 15 16 17
  18 19 20 21 22 23
  24 25 26 27 28 29
     30 31 32 33
        34 35
****************************************************************************/
static bv_pixel pixel_tile_hexa(const struct tile *ptile,
                                const struct player *pplayer,
                                bool knowledge)
{
  bv_pixel pixel;

  BV_SET_ALL(pixel);

  return pixel;
}

/************************************************************************//**
        -- --
     --  3  4 --
  --  7  8  9 10 --
  -- 13 14 15 16 --
  -- 19 20 21 22 --
  -- 25 26 27 28 --
     -- 31 32 --
        -- --
****************************************************************************/
static bv_pixel pixel_city_hexa(const struct tile *ptile,
                                const struct player *pplayer,
                                bool knowledge)
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

/************************************************************************//**
        -- --
     -- -- -- --
  -- -- -- -- -- --
  -- -- 14 15 -- --
  -- -- 20 21 -- --
  -- -- -- -- -- --
     -- -- -- --
        -- --
****************************************************************************/
static bv_pixel pixel_unit_hexa(const struct tile *ptile,
                                const struct player *pplayer,
                                bool knowledge)
{
  bv_pixel pixel;

  BV_CLR_ALL(pixel);
  BV_SET(pixel, 14);
  BV_SET(pixel, 15);
  BV_SET(pixel, 20);
  BV_SET(pixel, 21);

  return pixel;
}

/************************************************************************//**
         0 --
     --  3 --  5
  --  7 --  9 -- 11
  -- 13 -- 15 -- 17
  18 -- 20 -- 22 --
  24 -- 26 -- 28 --
     30 -- 32 --
        -- 35
****************************************************************************/
static bv_pixel pixel_fogofwar_hexa(const struct tile *ptile,
                                    const struct player *pplayer,
                                    bool knowledge)
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

/************************************************************************//**
   [W]        0  1       [N]
           2 -- --  5
        6 -- -- -- -- 11
  [SW] 12 -- -- -- -- 17 [NE]
       18 -- -- -- -- 23
       24 -- -- -- -- 29
          30 -- -- 33
   [S]       34 35       [E]
****************************************************************************/
static bv_pixel pixel_border_hexa(const struct tile *ptile,
                                  const struct player *pplayer,
                                  bool knowledge)
{
  bv_pixel pixel;
  struct tile *pnext;
  struct player *owner;

  BV_CLR_ALL(pixel);

  fc_assert_ret_val(ptile != NULL, pixel);

  if (NULL == ptile) {
    /* no tile */
    return pixel;
  }

  owner = mapimg.mapimg_tile_owner(ptile, pplayer, knowledge);
  if (NULL == owner) {
    /* no border */
    return pixel;
  }

  pnext = mapstep(&(wld.map), ptile, DIR8_WEST);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 0);
    BV_SET(pixel, 2);
    BV_SET(pixel, 6);
  }

  /* not used: DIR8_NORTHWEST */

  pnext = mapstep(&(wld.map), ptile, DIR8_NORTH);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 1);
    BV_SET(pixel, 5);
    BV_SET(pixel, 11);
  }

  pnext = mapstep(&(wld.map), ptile, DIR8_NORTHEAST);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 11);
    BV_SET(pixel, 17);
    BV_SET(pixel, 23);
    BV_SET(pixel, 29);
  }

  pnext = mapstep(&(wld.map), ptile, DIR8_EAST);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 29);
    BV_SET(pixel, 33);
    BV_SET(pixel, 35);
  }

  /* not used. DIR8_SOUTHEAST */

  pnext = mapstep(&(wld.map), ptile, DIR8_SOUTH);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 24);
    BV_SET(pixel, 30);
    BV_SET(pixel, 34);
  }

  pnext = mapstep(&(wld.map), ptile, DIR8_SOUTHWEST);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 6);
    BV_SET(pixel, 12);
    BV_SET(pixel, 18);
    BV_SET(pixel, 24);
  }

  return pixel;
}

/************************************************************************//**
  Base coordinates for the tiles on a hexa topology,
****************************************************************************/
static void base_coor_hexa(struct img *pimg, int *base_x, int *base_y,
                           int x, int y)
{
  int nat_x, nat_y;
  MAP_TO_NATIVE_POS(&nat_x, &nat_y, x, y);

  *base_x = nat_x * TILE_SIZE + ((nat_y % 2) ? TILE_SIZE / 2 : 0);
  *base_y = nat_y * TILE_SIZE;
}

/************************************************************************//**
         0  1  2  3
      4  5  6  7  8  9
  10 11 12 13 14 15 16 17
  18 19 20 21 22 23 24 25
     26 27 28 29 30 31
        32 33 34 35
****************************************************************************/
static bv_pixel pixel_tile_isohexa(const struct tile *ptile,
                                   const struct player *pplayer,
                                   bool knowledge)
{
  bv_pixel pixel;

  BV_SET_ALL(pixel);

  return pixel;
}

/************************************************************************//**
        -- -- -- --
     --  5  6  7  8 --
  -- 11 12 13 14 15 16 --
  -- 19 20 21 22 23 24 --
     -- 27 28 29 30 --
        -- -- -- --
****************************************************************************/
static bv_pixel pixel_city_isohexa(const struct tile *ptile,
                                   const struct player *pplayer,
                                   bool knowledge)
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

/************************************************************************//**
        -- -- -- --
     -- -- -- -- -- --
  -- -- -- 13 14 -- -- --
  -- -- -- 21 22 -- -- --
     -- -- -- -- -- --
        -- -- -- --
****************************************************************************/
static bv_pixel pixel_unit_isohexa(const struct tile *ptile,
                                   const struct player *pplayer,
                                   bool knowledge)
{
  bv_pixel pixel;

  BV_CLR_ALL(pixel);
  BV_SET(pixel, 13);
  BV_SET(pixel, 14);
  BV_SET(pixel, 21);
  BV_SET(pixel, 22);

  return pixel;
}

/************************************************************************//**
         0  1 -- --
      4 -- --  7  8 --
  -- -- 12 13 -- -- 16 17
  18 19 -- -- 22 23 -- --
     -- 27 28 -- -- 31
        -- -- 34 35
****************************************************************************/
static bv_pixel pixel_fogofwar_isohexa(const struct tile *ptile,
                                       const struct player *pplayer,
                                       bool knowledge)
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

/************************************************************************//**

               [N]

  [NW]       0  1  2  3      [E]
          4 -- -- -- --  9
      10 -- -- -- -- -- -- 17
      18 -- -- -- -- -- -- 25
         26 -- -- -- -- 31
  [W]       32 33 34 35      [SE]

               [S]
****************************************************************************/
static bv_pixel pixel_border_isohexa(const struct tile *ptile,
                                     const struct player *pplayer,
                                     bool knowledge)
{
  bv_pixel pixel;
  struct tile *pnext;
  struct player *owner;

  BV_CLR_ALL(pixel);

  fc_assert_ret_val(ptile != NULL, pixel);

  if (NULL == ptile) {
    /* no tile */
    return pixel;
  }

  owner = mapimg.mapimg_tile_owner(ptile, pplayer, knowledge);
  if (NULL == owner) {
    /* no border */
    return pixel;
  }

  pnext = mapstep(&(wld.map), ptile, DIR8_NORTH);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 0);
    BV_SET(pixel, 1);
    BV_SET(pixel, 2);
    BV_SET(pixel, 3);
  }

  /* not used: DIR8_NORTHEAST */

  pnext = mapstep(&(wld.map), ptile, DIR8_EAST);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 3);
    BV_SET(pixel, 9);
    BV_SET(pixel, 17);
  }

  pnext = mapstep(&(wld.map), ptile, DIR8_SOUTHEAST);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 25);
    BV_SET(pixel, 31);
    BV_SET(pixel, 35);
  }

  pnext = mapstep(&(wld.map), ptile, DIR8_SOUTH);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 32);
    BV_SET(pixel, 33);
    BV_SET(pixel, 34);
    BV_SET(pixel, 35);
  }

  /* not used: DIR8_SOUTHWEST */

  pnext = mapstep(&(wld.map), ptile, DIR8_WEST);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 18);
    BV_SET(pixel, 26);
    BV_SET(pixel, 32);
  }

  pnext = mapstep(&(wld.map), ptile, DIR8_NORTHWEST);
  if (!pnext || (mapimg.mapimg_tile_known(pnext, pplayer,
                                          knowledge) != TILE_UNKNOWN
                 && mapimg.mapimg_tile_owner(pnext, pplayer,
                                             knowledge) != owner)) {
    BV_SET(pixel, 0);
    BV_SET(pixel, 4);
    BV_SET(pixel, 10);
  }

  return pixel;
}

/************************************************************************//**
  Base coordinates for the tiles on a isometric hexa topology,
****************************************************************************/
static void base_coor_isohexa(struct img *pimg, int *base_x, int *base_y,
                              int x, int y)
{
  /* magic for iso-hexa */
  y -= x / 2;
  y += (pimg->mapsize.x - 1)/2;

  *base_x = x * TILE_SIZE;
  *base_y = y * TILE_SIZE + ((x % 2) ? 0 : TILE_SIZE / 2);
}

/*
 * ==============================================
 * additional functions (internal functions)
 * ==============================================
 */

/************************************************************************//**
  Convert the player bitvector to a string.
****************************************************************************/
static const char *bvplayers_str(const bv_player plrbv)
{
  static char buf[MAX_NUM_PLAYER_SLOTS + 1];
  int i;

  /* Don't print lots of unnecessary trailing zeroes */
  for (i = MAX_NUM_PLAYER_SLOTS-1; i >= 0; i--) {
    if (BV_ISSET(plrbv, i) || player_by_number(i)) {
      buf[i+1] = '\0';
      break;
    }
  }

  for (; i >= 0; i--) {
    buf[i] = BV_ISSET(plrbv, i) ? '1' : '0';
  }

  return buf;
}

/************************************************************************//**
  Return the number of players defined in a map image definition.
****************************************************************************/
static int bvplayers_count(const struct mapdef *pmapdef)
{
  int i, count = 0;

  switch (pmapdef->player.show) {
  case SHOW_NONE:    /* no player on the map */
    count = 0;
    break;
  case SHOW_HUMAN:   /* one map for each human player */
  case SHOW_EACH:    /* one map for each player */
  case SHOW_PLRNAME: /* the map of one selected player */
  case SHOW_PLRID:
    count = 1;
    break;
  case SHOW_PLRBV:   /* map showing only players given by a bitvector */
    count = 0;
    for (i = 0; i < MAX_NUM_PLAYER_SLOTS; i++) {
      if (BV_ISSET(pmapdef->player.plrbv, i)) {
        count++;
      }
    }
    break;
  case SHOW_ALL:     /* show all players in one map */
    count = player_count();
    break;
  }

  return count;
}

/*
 * ==============================================
 * image colors (internal functions)
 * ==============================================
 */

/************************************************************************//**
  Return rgbcolor for img_special
****************************************************************************/
static const struct rgbcolor *imgcolor_special(enum img_special imgcolor)
{
  static struct rgbcolor rgb_special[] = {
    { 255,   0,   0, NULL}, /* IMGCOLOR_ERROR */
    /* FIXME: 'ocean' and 'ground' colors are also used in the overview; the
     *        values are defined in colors.tilespec. */
    {   0,   0, 200, NULL}, /* IMGCOLOR_OCEAN */
    {   0, 200,   0, NULL}, /* IMGCOLOR_GROUND */
    {   0,   0,   0, NULL}, /* IMGCOLOR_BACKGROUND */
    { 255, 255, 255, NULL}, /* IMGCOLOR_TEXT */
  };

  fc_assert_ret_val(imgcolor >= 0 && imgcolor < ARRAY_SIZE(rgb_special),
                    &rgb_special[0]);

  return &rgb_special[imgcolor];
}

/************************************************************************//**
  Return rgbcolor for player.

  FIXME: nearly identical with get_player_color() in colors_common.c.
****************************************************************************/
static const struct rgbcolor *imgcolor_player(int plr_id)
{
  struct player *pplayer = player_by_number(plr_id);

  fc_assert_ret_val(pplayer != NULL, imgcolor_special(IMGCOLOR_ERROR));
  fc_assert_ret_val(pplayer->rgb != NULL,
                    imgcolor_special(IMGCOLOR_ERROR));

  return pplayer->rgb;
}

/************************************************************************//**
  Return rgbcolor for terrain.

  FIXME: nearly identical with get_terrain_color() in colors_common.c.
****************************************************************************/
static const struct rgbcolor
  *imgcolor_terrain(const struct terrain *pterrain)
{
  fc_assert_ret_val(pterrain != NULL, imgcolor_special(IMGCOLOR_ERROR));
  fc_assert_ret_val(pterrain->rgb != NULL, imgcolor_special(IMGCOLOR_ERROR));

  return pterrain->rgb;
}
