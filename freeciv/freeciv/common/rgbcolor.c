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

/* utility */
#include "fcintl.h"
#include "log.h"
#include "registry.h"

/* common */
#include "fc_interface.h"
#include "game.h"

#include "rgbcolor.h"

/************************************************************************//**
  Allocate new rgbcolor structure.
****************************************************************************/
struct rgbcolor *rgbcolor_new(int r, int g, int b)
{
  struct rgbcolor *prgbcolor;

  prgbcolor = fc_calloc(1, sizeof(*prgbcolor));
  prgbcolor->r = r;
  prgbcolor->g = g;
  prgbcolor->b = b;
  prgbcolor->color = NULL;

  return prgbcolor;
}

/************************************************************************//**
  Allocate new rgbcolor structure and make it copy of one given as input.
  Return new one.
****************************************************************************/
struct rgbcolor *rgbcolor_copy(const struct rgbcolor *prgbcolor)
{
  fc_assert_ret_val(prgbcolor != NULL, NULL);

  return rgbcolor_new(prgbcolor->r, prgbcolor->g, prgbcolor->b);
}

/************************************************************************//**
  Test whether two rgbcolor structures represent the exact same color value.
  (Does not attempt to determine whether they are visually distinguishable.)
****************************************************************************/
bool rgbcolors_are_equal(const struct rgbcolor *c1, const struct rgbcolor *c2)
{
  fc_assert_ret_val(c1 != NULL && c2 != NULL, FALSE);

  /* No check of cached 'color' member -- if values are equal, it should be
   * equivalent */
  return (c1->r == c2->r && c1->g == c2->g && c1->b == c2->b);
}

/************************************************************************//**
  Free rgbcolor structure.
****************************************************************************/
void rgbcolor_destroy(struct rgbcolor *prgbcolor)
{
  if (!prgbcolor) {
    return;
  }

  if (prgbcolor->color) {
    fc_funcs->gui_color_free(prgbcolor->color);
  }
  free(prgbcolor);
}

/************************************************************************//**
  Lookup an RGB color definition (<colorpath>.red, <colorpath>.green and
  <colorpath>.blue). Returns TRUE on success and FALSE on error.
****************************************************************************/
bool rgbcolor_load(struct section_file *file, struct rgbcolor **prgbcolor,
                   char *path, ...)
{
  int r, g, b;
  char colorpath[256];
  va_list args;

  fc_assert_ret_val(file != NULL, FALSE);
  fc_assert_ret_val(*prgbcolor == NULL, FALSE);

  va_start(args, path);
  fc_vsnprintf(colorpath, sizeof(colorpath), path, args);
  va_end(args);

  if (!secfile_lookup_int(file, &r, "%s.r", colorpath)
      || !secfile_lookup_int(file, &g, "%s.g", colorpath)
      || !secfile_lookup_int(file, &b, "%s.b", colorpath)) {
    /* One color value (red, green or blue) is missing. */
    return FALSE;
  }

  rgbcolor_check(colorpath, r, g, b);
  *prgbcolor = rgbcolor_new(r, g, b);

  return TRUE;
}

/************************************************************************//**
  Save an RGB color definition (<colorpath>.red, <colorpath>.green and
  <colorpath>.blue).
****************************************************************************/
void rgbcolor_save(struct section_file *file,
                   const struct rgbcolor *prgbcolor, char *path, ...)
{
  char colorpath[256];
  va_list args;

  fc_assert_ret(file != NULL);
  fc_assert_ret(prgbcolor != NULL);

  va_start(args, path);
  fc_vsnprintf(colorpath, sizeof(colorpath), path, args);
  va_end(args);

  secfile_insert_int(file, prgbcolor->r, "%s.r", colorpath);
  secfile_insert_int(file, prgbcolor->g, "%s.g", colorpath);
  secfile_insert_int(file, prgbcolor->b, "%s.b", colorpath);
}

/************************************************************************//**
  Convert a rgb color to a hex string (like 0xff0000 for red [255,  0,  0]).
****************************************************************************/
bool rgbcolor_to_hex(const struct rgbcolor *prgbcolor, char *hex,
                     size_t hex_len)
{
  fc_assert_ret_val(prgbcolor != NULL, FALSE);
  /* Needs a length greater than 7 ('#' + 6 hex digites and '\0'). */
  fc_assert_ret_val(hex_len > 7, FALSE);

  fc_assert_ret_val(0 <= prgbcolor->r && prgbcolor->r <= 255, FALSE);
  fc_assert_ret_val(0 <= prgbcolor->g && prgbcolor->g <= 255, FALSE);
  fc_assert_ret_val(0 <= prgbcolor->b && prgbcolor->b <= 255, FALSE);

  fc_snprintf(hex, hex_len, "#%06x",
              (prgbcolor->r * 256 + prgbcolor->g) * 256 + prgbcolor->b);

  return TRUE;
}

/************************************************************************//**
  Convert a hex string into a rgb color
****************************************************************************/
bool rgbcolor_from_hex(struct rgbcolor **prgbcolor, const char *hex)
{
  int rgb, r, g, b;
  char hex2[16];

  fc_assert_ret_val(*prgbcolor == NULL, FALSE);
  fc_assert_ret_val(hex != NULL, FALSE);

  if (hex[0] == '#') {
    hex++;
  }

  if (strlen(hex) != 6) {
    return FALSE;
  }

  fc_snprintf(hex2, sizeof(hex2), "0x%s", hex);
  if (!sscanf(hex2, "%x", &rgb)) {
    return FALSE;
  }

  r = rgb / 256 / 256;
  g = (rgb - r * 256 * 256) / 256;
  b = rgb % 256;

  *prgbcolor = rgbcolor_new(r, g, b);

  return TRUE;
}

/************************************************************************//**
  Return a number indicating the perceptual brightness of this color
  relative to others (larger is brighter).
****************************************************************************/
int rgbcolor_brightness_score(struct rgbcolor *prgbcolor)
{
  /* This simple scoring system taken from W3C "Techniques For Accessibility
   * Evaluation And Repair Tools", http://www.w3.org/TR/AERT#color-contrast
   *
   * "Color brightness is determined by the following formula:
   * ((Red value X 299) + (Green value X 587) + (Blue value X 114)) / 1000
   * Note: This algorithm is taken from a formula for converting RGB values to
   * YIQ [NTSC] values [specifically the Y component]. This brightness value
   * gives a perceived brightness for a color." */
  return (prgbcolor->r*299 + prgbcolor->g*587 + prgbcolor->b*114) / 1000;
}
