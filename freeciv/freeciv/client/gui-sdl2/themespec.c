/***********************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

/***********************************************************************
  Functions for handling the themespec files which describe
  the files and contents of themes.
  original author: David Pfitzner <dwp@mso.anu.edu.au>
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdio.h>
#include <stdlib.h> /* exit */
#include <string.h>

/* utility */
#include "capability.h"
#include "fcintl.h"
#include "log.h"
#include "registry.h"
#include "string_vector.h"

/* common */
#include "game.h"

/* client */
#include "citydlg_common.h"
#include "client_main.h" /* for client_state() */

/* gui-sdl2 */
#include "dialogs.h"
#include "gui_tilespec.h"
#include "mapview.h"
#include "sprite.h"

#include "themespec.h"

#define THEMESPEC_SDL2_CAPSTR "+Freeciv-2.6-sdl2-themespec duplicates_ok"
/*
 * Themespec capabilities acceptable to this program:
 *
 * +Freeciv-2.3-themespec  -  basic format for Freeciv versions 2.3.x;
 *                            required
 *
 * duplicates_ok  -  we can handle existence of duplicate tags
 *                   (lattermost tag which appears is used; themes which
 *                   have duplicates should specify "+duplicates_ok")
 */

#define SPEC_SDL2_CAPSTR "+Freeciv-2.6-sdl2-spec"
/*
 * Individual spec file capabilities acceptable to this program:
 * +Freeciv-2.3-spec  -  basic format for Freeciv versions 2.3.x; required
 */

#define THEMESPEC_SUFFIX ".themespec"

#if 0
/* TODO: may be useful for theme sprites, too */
struct named_sprites {
   ....
};
#endif /* 0 */

struct specfile {
  struct sprite *big_sprite;
  char *file_name;
};

#define SPECLIST_TAG specfile
#define SPECLIST_TYPE struct specfile
#include "speclist.h"

#define specfile_list_iterate(list, pitem) \
    TYPED_LIST_ITERATE(struct specfile, list, pitem)
#define specfile_list_iterate_end  LIST_ITERATE_END

/*
 * Information about an individual sprite. All fields except 'sprite' are
 * filled at the time of the scan of the specfile. 'Sprite' is
 * set/cleared on demand in theme_load_sprite/theme_unload_sprite.
 */
struct small_sprite {
  int ref_count;

  /* The sprite is in this file. */
  char *file;

  /* Or, the sprite is in this file at the location. */
  struct specfile *sf;
  int x, y, width, height;

  /* A little more (optional) data. */
  int hot_x, hot_y;

  struct sprite *sprite;
};

#define SPECLIST_TAG small_sprite
#define SPECLIST_TYPE struct small_sprite
#include "speclist.h"

#define small_sprite_list_iterate(list, pitem)                              \
  TYPED_LIST_ITERATE(struct small_sprite, list, pitem)
#define small_sprite_list_iterate_end LIST_ITERATE_END

#define SPECHASH_TAG small_sprite
#define SPECHASH_IKEY_TYPE char *
#define SPECHASH_IDATA_TYPE struct small_sprite *
#define SPECHASH_IKEY_VAL genhash_str_val_func
#define SPECHASH_IKEY_COMP genhash_str_comp_func
#define SPECHASH_IKEY_COPY genhash_str_copy_func
#define SPECHASH_IKEY_FREE genhash_str_free_func
#include "spechash.h"
#define small_sprite_hash_iterate(hash, tag_name, pitem)                    \
  TYPED_HASH_ITERATE(const char *, struct small_sprite *, hash,             \
                     tag_name, pitem)
#define small_sprite_hash_iterate_end HASH_ITERATE_END

struct theme {
  char name[512];
  int priority;

  char *font_filename;
  int default_font_size;

  struct specfile_list *specfiles;
  struct small_sprite_list *small_sprites;

  /*
   * This hash table maps themespec tags to struct small_sprites.
   */
  struct small_sprite_hash *sprite_hash;

/*  struct named_sprites sprites;*/

  struct theme_background_system *background_system;
  struct theme_color_system *color_system;
};

struct theme *theme = NULL;


/************************************************************************//**
  Return the name of the given theme.
****************************************************************************/
const char *theme_get_name(const struct theme *t)
{
  return t->name;
}

/************************************************************************//**
  Return the path within the data directories where the font file can be
  found.  (It is left up to the GUI code to load and unload this file.)
****************************************************************************/
const char *theme_font_filename(const struct theme *t)
{
  return t->font_filename;
}

/************************************************************************//**
  Return the default font size.
****************************************************************************/
int theme_default_font_size(const struct theme *t)
{
  return t->default_font_size;
}

/************************************************************************//**
  Initialize.
****************************************************************************/
static struct theme *theme_new(void)
{
  struct theme *t = fc_calloc(1, sizeof(*t));

  t->specfiles = specfile_list_new();
  t->small_sprites = small_sprite_list_new();

  return t;
}

/************************************************************************//**
  Returns a static list of themes available on the system by
  searching all data directories for files matching THEMESPEC_SUFFIX.
  The list is NULL-terminated.
****************************************************************************/
const char **get_theme_list(void)
{
  static const char **themes = NULL;

  if (!themes) {
    /* Note: this means you must restart the client after installing a new
       theme. */
    struct strvec *list = fileinfolist(get_data_dirs(), THEMESPEC_SUFFIX);
    int count = 0;

    themes = fc_malloc((1 + strvec_size(list)) * sizeof(*themes));
    strvec_iterate(list, file) {
      struct theme *t = theme_read_toplevel(file);

      if (t) {
        themes[count++] = fc_strdup(file);
        theme_free(t);
      }
    } strvec_iterate_end;
    themes[count] = NULL;
    strvec_destroy(list);
  }

  return themes;
}

/************************************************************************//**
  Gets full filename for themespec file, based on input name.
  Returned data is allocated, and freed by user as required.
  Input name may be null, in which case uses default.
  Falls back to default if can't find specified name;
  dies if can't find default.
****************************************************************************/
static char *themespec_fullname(const char *theme_name)
{
  if (theme_name) {
    char fname[strlen(theme_name) + strlen(THEMESPEC_SUFFIX) + 1], *dname;

    fc_snprintf(fname, sizeof(fname),
                "%s%s", theme_name, THEMESPEC_SUFFIX);

    dname = fname;

    if (dname) {
      return fc_strdup(dname);
    }
  }

  return NULL;
}

/************************************************************************//**
  Checks options in filename match what we require and support.
  Die if not.
  'which' should be "themespec" or "spec".
****************************************************************************/
static bool check_themespec_capabilities(struct section_file *file,
                                         const char *which,
                                         const char *us_capstr,
                                         const char *filename)
{
  const char *file_capstr = secfile_lookup_str(file, "%s.options", which);

  if (NULL == file_capstr) {
    log_debug("\"%s\": %s file doesn't have capability string",
              filename, which);
    return FALSE;
  }

  if (!has_capabilities(us_capstr, file_capstr)) {
    log_debug("\"%s\": %s file appears incompatible:", filename, which);
    log_debug("  datafile options: %s", file_capstr);
    log_debug("  supported options: %s", us_capstr);

    return FALSE;
  }

  if (!has_capabilities(file_capstr, us_capstr)) {
    log_debug("\"%s\": %s file requires option(s)"
              " that client doesn't support:", filename, which);
    log_debug("  datafile options: %s", file_capstr);
    log_debug("  supported options: %s", us_capstr);

    return FALSE;
  }

  return TRUE;
}

/************************************************************************//**
  Frees the themespec toplevel data, in preparation for re-reading it.

  See themespec_read_toplevel().
****************************************************************************/
static void theme_free_toplevel(struct theme *t)
{
  if (t->font_filename) {
    FC_FREE(t->font_filename);
  }

  t->default_font_size = 0;

  if (t->background_system) {
    theme_background_system_free(t->background_system);
    t->background_system = NULL;
  }

  if (t->color_system) {
    theme_color_system_free(t->color_system);
    t->color_system = NULL;
  }
}

/************************************************************************//**
  Free resources allocated for the theme.
****************************************************************************/
void theme_free(struct theme *ftheme)
{
  if (ftheme != NULL) {
    theme_free_sprites(ftheme);
    theme_free_toplevel(ftheme);
    specfile_list_destroy(ftheme->specfiles);
    small_sprite_list_destroy(ftheme->small_sprites);
    free(ftheme);
  }
}

/************************************************************************//**
  Read a new themespec in when first starting the game.

  Call this function with the (guessed) name of the theme, when
  starting the client.
****************************************************************************/
void themespec_try_read(const char *theme_name)
{
  if (!(theme = theme_read_toplevel(theme_name))) {
    struct strvec *list = fileinfolist(get_data_dirs(), THEMESPEC_SUFFIX);

    strvec_iterate(list, file) {
      struct theme *t = theme_read_toplevel(file);

      if (t) {
        if (!theme || t->priority > theme->priority) {
          theme = t;
        } else {
          theme_free(t);
        }
      }
    } strvec_iterate_end;
    strvec_destroy(list);

    if (!theme) {
      log_fatal(_("No usable default theme found, aborting!"));
      exit(EXIT_FAILURE);
    }

    log_verbose("Trying theme \"%s\".", theme->name);
  }
/*  sz_strlcpy(gui_sdl2_default_theme_name, theme_get_name(theme));*/
}

/************************************************************************//**
  Read a new themespec in from scratch.

  Unlike the initial reading code, which reads pieces one at a time,
  this gets rid of the old data and reads in the new all at once.  If the
  new theme fails to load the old theme may be reloaded; otherwise the
  client will exit.  If a NULL name is given the current theme will be
  reread.

  It will also call the necessary functions to redraw the graphics.
****************************************************************************/
void themespec_reread(const char *new_theme_name)
{
  struct tile *center_tile;
  enum client_states state = client_state();
  const char *name = new_theme_name ? new_theme_name : theme->name;
  char theme_name[strlen(name) + 1], old_name[strlen(theme->name) + 1];

  /* Make local copies since these values may be freed down below */
  sz_strlcpy(theme_name, name);
  sz_strlcpy(old_name, theme->name);

  log_normal(_("Loading theme \"%s\"."), theme_name);

  /* Step 0:  Record old data.
   *
   * We record the current mapcanvas center, etc.
   */
  center_tile = get_center_tile_mapcanvas();

  /* Step 1:  Cleanup.
   *
   * We free all old data in preparation for re-reading it.
   */
  theme_free_sprites(theme);
  theme_free_toplevel(theme);

  /* Step 2:  Read.
   *
   * We read in the new theme.  This should be pretty straightforward.
   */
  if (!(theme = theme_read_toplevel(theme_name))) {
    if (!(theme = theme_read_toplevel(old_name))) {
      /* Always fails. */
      fc_assert_exit_msg(NULL != theme,
                         "Failed to re-read the currently loaded theme.");
    }
  }
/*  sz_strlcpy(gui_sdl2_default_theme_name, theme->name);*/
  theme_load_sprites(theme);

  /* Step 3: Setup
   *
   * This is a seriously sticky problem.  On startup, we build a hash
   * from all the sprite data. Then, when we connect to a server, the
   * server sends us ruleset data a piece at a time and we use this data
   * to assemble the sprite structures.  But if we change while connected
   *  we have to reassemble all of these.  This should just involve
   * calling themespec_setup_*** on everything.  But how do we tell what
   * "everything" is?
   *
   * The below code just does things straightforwardly, by setting up
   * each possible sprite again.  Hopefully it catches everything, and
   * doesn't mess up too badly if we change themes while not connected
   * to a server.
   */
  if (state < C_S_RUNNING) {
    /* The ruleset data is not sent until this point. */
    return;
  }

  /* Step 4:  Draw.
   *
   * Do any necessary redraws.
   */
  popdown_all_game_dialogs();
  generate_citydlg_dimensions();
/*  theme_changed();*/
  can_slide = FALSE;
  center_tile_mapcanvas(center_tile);
  /* update_map_canvas_visible forces a full redraw.  Otherwise with fast
   * drawing we might not get one.  Of course this is slower. */
  update_map_canvas_visible();
  can_slide = TRUE;
}

/************************************************************************//**
  Loads the given graphics file (found in the data path) into a newly
  allocated sprite.
****************************************************************************/
static struct sprite *load_gfx_file(const char *gfx_filename)
{
  const char **gfx_fileexts = gfx_fileextensions(), *gfx_fileext;
  struct sprite *s;

  /* Try out all supported file extensions to find one that works. */
  while ((gfx_fileext = *gfx_fileexts++)) {
    const char *real_full_name;
    char full_name[strlen(gfx_filename) + strlen(".")
                   + strlen(gfx_fileext) + 1];

    sprintf(full_name, "%s.%s", gfx_filename, gfx_fileext);
    if ((real_full_name = fileinfoname(get_data_dirs(), full_name))) {
      log_debug("trying to load gfx file \"%s\".", real_full_name);
      s = load_gfxfile(real_full_name);
      if (s) {
        return s;
      }
    }
  }

  log_error("Could not load gfx file \"%s\".", gfx_filename);
  return NULL;
}

/************************************************************************//**
  Ensure that the big sprite of the given spec file is loaded.
****************************************************************************/
static void ensure_big_sprite(struct specfile *sf)
{
  struct section_file *file;
  const char *gfx_filename;

  if (sf->big_sprite) {
    /* Looks like it's already loaded. */
    return;
  }

  /* Otherwise load it.  The big sprite will sometimes be freed and will have
   * to be reloaded, but most of the time it's just loaded once, the small
   * sprites are extracted, and then it's freed. */
  if (!(file = secfile_load(sf->file_name, TRUE))) {
    log_fatal(_("Could not open '%s':\n%s"), sf->file_name, secfile_error());
    exit(EXIT_FAILURE);
  }

  if (!check_themespec_capabilities(file, "spec",
                                    SPEC_SDL2_CAPSTR, sf->file_name)) {
    exit(EXIT_FAILURE);
  }

  gfx_filename = secfile_lookup_str(file, "file.gfx");

  sf->big_sprite = load_gfx_file(gfx_filename);

  if (!sf->big_sprite) {
    log_fatal("Could not load gfx file for the spec file \"%s\".",
              sf->file_name);
    exit(EXIT_FAILURE);
  }
  secfile_destroy(file);
}

/************************************************************************//**
  Scan all sprites declared in the given specfile.  This means that the
  positions of the sprites in the big_sprite are saved in the
  small_sprite structs.
****************************************************************************/
static void scan_specfile(struct theme *t, struct specfile *sf,
                          bool duplicates_ok)
{
  struct section_file *file;
  struct section_list *sections;
  int i;

  if (!(file = secfile_load(sf->file_name, TRUE))) {
    log_fatal(_("Could not open '%s':\n%s"), sf->file_name, secfile_error());
    exit(EXIT_FAILURE);
  }
  if (!check_themespec_capabilities(file, "spec",
                                    SPEC_SDL2_CAPSTR, sf->file_name)) {
    exit(EXIT_FAILURE);
  }

  /* currently unused */
  (void) secfile_entry_by_path(file, "info.artists");

  sections = secfile_sections_by_name_prefix(file, "grid_");

  if (NULL != sections) {
    section_list_iterate(sections, psection) {
      int j, k;
      int x_top_left, y_top_left, dx, dy;
      int pixel_border;
      const char *gridname = section_name(psection);

      pixel_border = secfile_lookup_int_default(file, 0, "%s.pixel_border",
                                                gridname);

      if (!secfile_lookup_int(file, &x_top_left, "%s.x_top_left", gridname)
          || !secfile_lookup_int(file, &y_top_left,
                                 "%s.y_top_left", gridname)
          || !secfile_lookup_int(file, &dx, "%s.dx", gridname)
          || !secfile_lookup_int(file, &dy, "%s.dy", gridname)) {
        log_error("Grid \"%s\" invalid: %s", gridname, secfile_error());
        continue;
      }

      j = -1;
      while (NULL != secfile_entry_lookup(file, "%s.tiles%d.tag",
                                          gridname, ++j)) {
        struct small_sprite *ss;
        int row, column;
        int x1, y1;
        const char **tags;
        size_t num_tags;
        int hot_x, hot_y;

        if (!secfile_lookup_int(file, &row, "%s.tiles%d.row", gridname, j)
            || !secfile_lookup_int(file, &column,
                                   "%s.tiles%d.column", gridname, j)
            || !(tags = secfile_lookup_str_vec(file, &num_tags,
                                               "%s.tiles%d.tag",
                                               gridname, j))) {
          log_error("Small sprite \"%s.tiles%d\" invalid: %s",
                    gridname, j, secfile_error());
          continue;
        }
        hot_x = secfile_lookup_int_default(file, 0, "%s.tiles%d.hot_x",
                                           gridname, j);
        hot_y = secfile_lookup_int_default(file, 0, "%s.tiles%d.hot_y",
                                           gridname, j);

        /* there must be at least 1 because of the while(): */
        fc_assert_action(num_tags > 0, continue);

        x1 = x_top_left + (dx + pixel_border) * column;
        y1 = y_top_left + (dy + pixel_border) * row;

        ss = fc_malloc(sizeof(*ss));
        ss->ref_count = 0;
        ss->file = NULL;
        ss->x = x1;
        ss->y = y1;
        ss->width = dx;
        ss->height = dy;
        ss->sf = sf;
        ss->sprite = NULL;
        ss->hot_x = hot_x;
        ss->hot_y = hot_y;

        small_sprite_list_prepend(t->small_sprites, ss);

        if (!duplicates_ok) {
          for (k = 0; k < num_tags; k++) {
            if (!small_sprite_hash_insert(t->sprite_hash, tags[k], ss)) {
              log_error("warning: already have a sprite for \"%s\".",
                        tags[k]);
            }
          }
        } else {
          for (k = 0; k < num_tags; k++) {
            (void) small_sprite_hash_replace(t->sprite_hash, tags[k], ss);
          }
        }

        FC_FREE(tags);
      }
    } section_list_iterate_end;
    section_list_destroy(sections);
  }

  /* Load "extra" sprites.  Each sprite is one file. */
  i = -1;
  while (NULL != secfile_lookup_str(file, "extra.sprites%d.tag", ++i)) {
    struct small_sprite *ss;
    const char **tags;
    const char *filename;
    size_t num_tags, k;
    int hot_x, hot_y;

    if (!(tags = secfile_lookup_str_vec(file, &num_tags,
                                        "extra.sprites%d.tag", i))
        || !(filename = secfile_lookup_str(file,
                                           "extra.sprites%d.file", i))) {
      log_error("Small sprite \"extra.sprites%d\" invalid: %s",
                i, secfile_error());
      continue;
    }

    hot_x = secfile_lookup_int_default(file, 0, "extra.sprites%d.hot_x", i);
    hot_y = secfile_lookup_int_default(file, 0, "extra.sprites%d.hot_y", i);

    ss = fc_malloc(sizeof(*ss));
    ss->ref_count = 0;
    ss->file = fc_strdup(filename);
    ss->sf = NULL;
    ss->sprite = NULL;
    ss->hot_x = hot_x;
    ss->hot_y = hot_y;

    small_sprite_list_prepend(t->small_sprites, ss);

    if (!duplicates_ok) {
      for (k = 0; k < num_tags; k++) {
        if (!small_sprite_hash_insert(t->sprite_hash, tags[k], ss)) {
          log_error("warning: already have a sprite for \"%s\".", tags[k]);
        }
      }
    } else {
      for (k = 0; k < num_tags; k++) {
        (void) small_sprite_hash_replace(t->sprite_hash, tags[k], ss);
      }
    }
    FC_FREE(tags);
  }

  secfile_check_unused(file);
  secfile_destroy(file);
}

/************************************************************************//**
  Returns the correct name of the gfx file (with path and extension)
  Must be free'd when no longer used
****************************************************************************/
char *themespec_gfx_filename(const char *gfx_filename)
{
  const char  *gfx_current_fileext;
  const char **gfx_fileexts = gfx_fileextensions();

  while ((gfx_current_fileext = *gfx_fileexts++)) {
    const char *real_full_name;
    char *full_name =
      fc_malloc(strlen(gfx_filename) + strlen(".")
                + strlen(gfx_current_fileext) + 1);

    sprintf(full_name, "%s.%s", gfx_filename, gfx_current_fileext);
    real_full_name = fileinfoname(get_data_dirs(), full_name);
    FC_FREE(full_name);
    if (real_full_name) {
      return fc_strdup(real_full_name);
    }
  }

  log_fatal("Couldn't find a supported gfx file extension for \"%s\".",
            gfx_filename);

  exit(EXIT_FAILURE);
  return NULL;
}

/************************************************************************//**
  Finds and reads the toplevel themespec file based on given name.
  Sets global variables, including tile sizes and full names for
  intro files.
****************************************************************************/
struct theme *theme_read_toplevel(const char *theme_name)
{
  struct section_file *file;
  char *fname;
  int i;
  size_t num_spec_files;
  const char **spec_filenames;
  const char *file_capstr;
  bool duplicates_ok;
  struct theme *t = theme_new();
  const char *langname;
  const char *filename, *c;

  fname = themespec_fullname(theme_name);
  if (!fname) {
    log_error("Can't find theme \"%s\".", theme_name);
    theme_free(t);
    return NULL;
  }
  log_verbose("themespec file is \"%s\".", fname);

  if (!(file = secfile_load(fname, TRUE))) {
    log_error("Could not open '%s':\n%s", fname, secfile_error());
    FC_FREE(fname);
    theme_free(t);

    return NULL;
  }

  if (!check_themespec_capabilities(file, "themespec",
                                    THEMESPEC_SDL2_CAPSTR, fname)) {
    secfile_destroy(file);
    FC_FREE(fname);
    theme_free(t);

    return NULL;
  }

  file_capstr = secfile_lookup_str(file, "themespec.options");
  duplicates_ok = has_capabilities("+duplicates_ok", file_capstr);

  (void) secfile_entry_by_path(file, "themespec.name"); /* currently unused */

  sz_strlcpy(t->name, theme_name);
  t->priority = secfile_lookup_int_default(file, 0, "themespec.priority");

  langname = setup_langname();
  if (langname) {
    if (strstr(langname, "zh_CN") != NULL) {
      c = secfile_lookup_str(file, "themespec.font_file_zh_CN");
    } else if (strstr(langname, "ja") != NULL) {
      c = secfile_lookup_str(file, "themespec.font_file_ja");
    } else if (strstr(langname, "ko") != NULL) {
      c = secfile_lookup_str(file, "themespec.font_file_ko");
    } else {
      c = secfile_lookup_str(file, "themespec.font_file");
    }
  } else {
    c = secfile_lookup_str(file, "themespec.font_file");
  }
  if ((filename = fileinfoname(get_data_dirs(), c))) {
    t->font_filename = fc_strdup(filename);
  } else {
    log_fatal("Could not open font: %s", c);
    secfile_destroy(file);
    FC_FREE(fname);
    theme_free(t);

    return NULL;
  }
  log_debug("theme font file %s", t->font_filename);

  t->default_font_size = secfile_lookup_int_default(file, 10, "themespec.default_font_size");
  log_debug("theme default font size %d", t->default_font_size);

  spec_filenames = secfile_lookup_str_vec(file, &num_spec_files,
                                          "themespec.files");
  if (NULL == spec_filenames || 0 == num_spec_files) {
    log_error("No theme graphics files specified in \"%s\"", fname);
    secfile_destroy(file);
    FC_FREE(fname);
    theme_free(t);

    return NULL;
  }

  fc_assert(t->sprite_hash == NULL);
  t->sprite_hash = small_sprite_hash_new();
  for (i = 0; i < num_spec_files; i++) {
    struct specfile *sf = fc_malloc(sizeof(*sf));

    log_debug("spec file %s", spec_filenames[i]);

    sf->big_sprite = NULL;
    filename = fileinfoname(get_data_dirs(), spec_filenames[i]);
    if (!filename) {
      log_error("Can't find spec file \"%s\".", spec_filenames[i]);
      secfile_destroy(file);
      FC_FREE(fname);
      theme_free(t);

      return NULL;
    }
    sf->file_name = fc_strdup(filename);
    scan_specfile(t, sf, duplicates_ok);

    specfile_list_prepend(t->specfiles, sf);
  }
  FC_FREE(spec_filenames);

  t->background_system = theme_background_system_read(file);
  t->color_system = theme_color_system_read(file);

  secfile_check_unused(file);

  secfile_destroy(file);
  log_verbose("finished reading \"%s\".", fname);
  FC_FREE(fname);

  return t;
}

/************************************************************************//**
  Loads the sprite. If the sprite is already loaded a reference
  counter is increased. Can return NULL if the sprite couldn't be
  loaded.
****************************************************************************/
static struct sprite *theme_load_sprite(struct theme *t, const char *tag_name)
{
  /* Lookup information about where the sprite is found. */
  struct small_sprite *ss;

  log_debug("theme_load_sprite(tag='%s')", tag_name);
  if (!small_sprite_hash_lookup(t->sprite_hash, tag_name, &ss)) {
    return NULL;
  }

  fc_assert_ret_val(ss->ref_count >= 0, NULL);

  if (!ss->sprite) {
    /* If the sprite hasn't been loaded already, then load it. */
    fc_assert_ret_val(ss->ref_count == 0, NULL);
    if (ss->file) {
      ss->sprite = load_gfx_file(ss->file);
      if (!ss->sprite) {
        log_fatal("Couldn't load gfx file \"%s\" for sprite '%s'.",
                  ss->file, tag_name);
        exit(EXIT_FAILURE);
      }
    } else {
      int sf_w, sf_h;

      ensure_big_sprite(ss->sf);
      get_sprite_dimensions(ss->sf->big_sprite, &sf_w, &sf_h);
      if (ss->x < 0 || ss->x + ss->width > sf_w
          || ss->y < 0 || ss->y + ss->height > sf_h) {
        log_error("Sprite '%s' in file \"%s\" isn't within the image!",
                  tag_name, ss->sf->file_name);
        return NULL;
      }
      ss->sprite =
	crop_sprite(ss->sf->big_sprite, ss->x, ss->y, ss->width, ss->height,
		    NULL, -1, -1, 1.0, FALSE);
    }
  }

  /* Track the reference count so we know when to free the sprite. */
  ss->ref_count++;

  return ss->sprite;
}

/************************************************************************//**
  Unloads the sprite. Decrease the reference counter. If the last
  reference is removed the sprite is freed.
****************************************************************************/
static void theme_unload_sprite(struct theme *t, const char *tag_name)
{
  struct small_sprite *ss;

  small_sprite_hash_lookup(t->sprite_hash, tag_name, &ss);
  fc_assert_ret(ss);
  fc_assert_ret(ss->ref_count >= 1);
  fc_assert_ret(ss->sprite);

  ss->ref_count--;

  if (ss->ref_count == 0) {
    /* Nobody's using the sprite anymore, so we should free it.  We know
     * where to find it if we need it again. */
    log_debug("freeing sprite '%s'.", tag_name);
    free_sprite(ss->sprite);
    ss->sprite = NULL;
  }
}

/* Not very safe, but convenient: */
#define SET_SPRITE(field, tag)					  \
  do {								  \
    t->sprites.field = theme_load_sprite(t, tag);		  \
    fc_assert_exit_msg(NULL != t->sprites.field,                  \
                       "Sprite tag '%s' missing.", tag);          \
  } while (FALSE)

/* Sets sprites.field to tag or (if tag isn't available) to alt */
#define SET_SPRITE_ALT(field, tag, alt)					    \
  do {									    \
    t->sprites.field = theme_load_sprite(t, tag);			    \
    if (!t->sprites.field) {						    \
      t->sprites.field = theme_load_sprite(t, alt);			    \
    }									    \
    fc_assert_exit_msg(NULL != t->sprites.field,                            \
                       "Sprite tag '%s' and alternate '%s' are "            \
                       "both missing.", tag, alt);                          \
  } while (FALSE)

/* Sets sprites.field to tag, or NULL if not available */
#define SET_SPRITE_OPT(field, tag) \
  t->sprites.field = theme_load_sprite(t, tag)

#define SET_SPRITE_ALT_OPT(field, tag, alt)				    \
  do {									    \
    t->sprites.field = theme_lookup_sprite_tag_alt(t, LOG_VERBOSE, tag, alt,\
						   "sprite", #field);	    \
  } while (FALSE)

/************************************************************************//**
  Initialize 'sprites' structure based on hardwired tags which the
  client always requires.
****************************************************************************/
static void theme_lookup_sprite_tags(struct theme *t)
{
  /* the 'sprites' structure is currently not used, for now we call some
   * functions in gui_tilespec.c instead */

  tilespec_setup_theme();
  tilespec_setup_city_gfx();
  tilespec_setup_city_icons();
}

/************************************************************************//**
  Frees any internal buffers which are created by theme_load_sprite. Should
  be called after the last (for a given period of time) theme_load_sprite
  call.  This saves a fair amount of memory, but it will take extra time
  the next time we start loading sprites again.
****************************************************************************/
static void theme_finish_loading_sprites(struct theme *t)
{
  specfile_list_iterate(t->specfiles, sf) {
    if (sf->big_sprite) {
      free_sprite(sf->big_sprite);
      sf->big_sprite = NULL;
    }
  } specfile_list_iterate_end;
}

/************************************************************************//**
  Load the tiles; requires themespec_read_toplevel() called previously.
  Leads to tile_sprites being allocated and filled with pointers
  to sprites.   Also sets up and populates sprite_hash, and calls func
  to initialize 'sprites' structure.
****************************************************************************/
void theme_load_sprites(struct theme *t)
{
  theme_lookup_sprite_tags(t);
  theme_finish_loading_sprites(t);
}

/************************************************************************//**
  Lookup sprite to match tag, or else to match alt if don't find,
  or else return NULL, and emit log message.
****************************************************************************/
struct sprite *theme_lookup_sprite_tag_alt(struct theme *t,
                                           enum log_level level,
                                           const char *tag, const char *alt,
                                           const char *what,
                                           const char *name)
{
  struct sprite *sp;

  /* (should get sprite_hash before connection) */
  fc_assert_ret_val_msg(NULL != t->sprite_hash, NULL,
                        "attempt to lookup for %s \"%s\" before "
                        "sprite_hash setup", what, name);

  sp = theme_load_sprite(t, tag);
  if (sp) {
    return sp;
  }

  sp = theme_load_sprite(t, alt);
  if (sp) {
    log_verbose("Using alternate graphic \"%s\" (instead of \"%s\") "
                "for %s \"%s\".", alt, tag, what, name);
    return sp;
  }

  log_base(level, "Don't have graphics tags \"%s\" or \"%s\" for %s \"%s\".",
           tag, alt, what, name);
  if (LOG_FATAL >= level) {
    exit(EXIT_FAILURE);
  }

  return NULL;
}

#define FULL_TILE_X_OFFSET ((t->normal_tile_width - t->full_tile_width) / 2)
#define FULL_TILE_Y_OFFSET (t->normal_tile_height - t->full_tile_height)

#define ADD_SPRITE(s, draw_fog, x_offset, y_offset)			    \
  (fc_assert(s != NULL),						    \
   sprs->sprite = s,							    \
   sprs->foggable = (draw_fog && t->fogstyle == FOG_AUTO),		    \
   sprs->offset_x = x_offset,						    \
   sprs->offset_y = y_offset,						    \
   sprs++)
#define ADD_SPRITE_SIMPLE(s) ADD_SPRITE(s, TRUE, 0, 0)
#define ADD_SPRITE_FULL(s)						    \
  ADD_SPRITE(s, TRUE, FULL_TILE_X_OFFSET, FULL_TILE_Y_OFFSET)

/************************************************************************//**
  Unload all sprites from theme sprite hash (the hash itself is left intact).
****************************************************************************/
static void theme_unload_all_sprites(struct theme *t)
{
  if (t->sprite_hash) {
    small_sprite_hash_iterate(t->sprite_hash, tag_name, ss) {
      while (ss->ref_count > 0) {
        theme_unload_sprite(t, tag_name);
      }
    } small_sprite_hash_iterate_end;
  }
}

/************************************************************************//**
  Free all sprites for the theme.
****************************************************************************/
void theme_free_sprites(struct theme *t)
{
  log_debug("theme_free_sprites()");

  theme_unload_all_sprites(t);

  if (t->sprite_hash) {
    small_sprite_hash_destroy(t->sprite_hash);
    t->sprite_hash = NULL;
  }

  small_sprite_list_iterate(t->small_sprites, ss) {
    small_sprite_list_remove(t->small_sprites, ss);
    if (ss->file) {
      FC_FREE(ss->file);
    }
    fc_assert(ss->sprite == NULL);
    FC_FREE(ss);
  } small_sprite_list_iterate_end;

  specfile_list_iterate(t->specfiles, sf) {
    specfile_list_remove(t->specfiles, sf);
    FC_FREE(sf->file_name);
    if (sf->big_sprite) {
      free_sprite(sf->big_sprite);
      sf->big_sprite = NULL;
    }
    FC_FREE(sf);
  } specfile_list_iterate_end;

#if 0
  /* TODO: may be useful for theme sprites, too */
  sprite_vector_iterate(&t->sprites.****, psprite) {
    free_sprite(*psprite);
  } sprite_vector_iterate_end;
  sprite_vector_free(&t->sprites.****);
#endif /* 0 */

  /* the 'sprites' structure is currently not used, for now we call some
   * functions in gui_tilespec.c instead */

  tilespec_free_theme();
  tilespec_free_city_gfx();
  tilespec_free_city_icons();
}

/************************************************************************//**
  Return the theme's background system.
****************************************************************************/
struct theme_background_system *theme_get_background_system(const struct theme *t)
{
  return t->background_system;
}

/************************************************************************//**
  Return the theme's color system.
****************************************************************************/
struct theme_color_system *theme_get_color_system(const struct theme *t)
{
  return t->color_system;
}
