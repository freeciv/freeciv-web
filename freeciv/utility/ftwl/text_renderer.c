/**********************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Project
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

#include <assert.h>
#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "log.h"
#include "mem.h"

#include "widget_p.h"

struct glyph_data {
  struct ct_point offset;
  struct FT_Bitmap_ *bitmap;
};

struct tr_string_data {
  int num_glyphs;
  struct glyph_data *glyphs;
  struct ct_size size;
};

static FT_Library library;

#define FT_CEIL(X)      (((X + 63) & -64) / 64)

static int HALO1_DX[] = { 0, -1,  1,  0};
static int HALO1_DY[] = {-1,  0,  0,  1};
#define HALO1_N 4
static int HALO2_DX[] = {-1,  0,  1, -1,  1, -1,  0,  1};
static int HALO2_DY[] = {-1, -1, -1,  0,  0,  1,  1,  1};
#define HALO2_N 8
static int HALO3_DX[] = { 0, -1,  0,  1, -2, -1,  1,  2, -1,  0,  1,  0};
static int HALO3_DY[] = {-2, -1, -1, -1,  0,  0,  0,  0,  1,  1,  1,  2};
#define HALO3_N 12
static int HALO4_DX[] = {-1,  0,  1, -2, -1,  0,  1,  2, -2, -1,  1,  2, -2, -1,  0,  1,  2, -1,  0,  1};
static int HALO4_DY[] = {-2, -2, -2, -1, -1, -1, -1, -1,  0,  0,  0,  0,  1,  1,  1,  1,  1,  2,  2,  2};
#define HALO4_N 20
static int HALO5_DX[] = { 0, -2, -1,  0,  1,  2, -2, -1,  0,  1,  2, -3, -2, -1,  1,  2,  3, -2, -1,  0,  1,  2, -2, -1,  0,  1,  2,  0};
static int HALO5_DY[] = {-3, -2, -2, -2, -2, -2, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  3};
#define HALO5_N 28
static int HALO6_DX[] = {-1,  0,  1, -2, -1,  0,  1,  2, -3, -2, -1,  0,  1,  2,  3, -3, -2, -1,  1,  2,  3, -3, -2, -1,  0,  1,  2,  3, -2, -1,  0,  1,  2, -1,  0,  1};
static int HALO6_DY[] = {-3, -3, -3, -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  3,  3,  3};
#define HALO6_N 36

static int *HALO_DX[] =
    { NULL, HALO1_DX, HALO2_DX, HALO3_DX, HALO4_DX, HALO5_DX, HALO6_DX };
static int *HALO_DY[] =
    { NULL, HALO1_DY, HALO2_DY, HALO3_DY, HALO4_DY, HALO5_DY, HALO6_DY };
static int HALO_N[]={0,HALO1_N,HALO2_N,HALO3_N,HALO4_N,HALO5_N,HALO6_N};

/*************************************************************************
  ...
*************************************************************************/
static struct FT_Bitmap_ *clone_bitmap(const struct FT_Bitmap_ *orig)
{
  if (orig->rows == 0 || orig->width == 0) {
    return NULL;
  } else {
    struct FT_Bitmap_ *result = fc_malloc(sizeof(*result));
    size_t buffer_size = orig->rows * orig->pitch;

    *result = *orig;
    result->buffer = fc_malloc(buffer_size);
    memcpy(result->buffer, orig->buffer, buffer_size);
    return result;
  }
}

/*************************************************************************
  ...
*************************************************************************/
static void free_bitmap(struct FT_Bitmap_ *bitmap)
{
  free(bitmap->buffer);
  free(bitmap);
}

/*************************************************************************
  ...
*************************************************************************/
static int get_height(FT_Face face)
{
  FT_Fixed scale = face->size->metrics.y_scale;
  int ascent = FT_CEIL(FT_MulFix(face->bbox.yMax, scale));
  int descent = FT_CEIL(FT_MulFix(face->bbox.yMin, scale));

  return ascent - descent + 1;
}

/*************************************************************************
  ...
*************************************************************************/
static int get_asc(FT_Face face)
{
  FT_Fixed scale = face->size->metrics.y_scale;
  return FT_CEIL(FT_MulFix(face->bbox.yMax, scale));
}

/*************************************************************************
  ...
*************************************************************************/
void tr_init(void)
{
  FT_Error error;

  error = FT_Init_FreeType(&library);
  assert(!error);
}

/*************************************************************************
  ...
*************************************************************************/
void tr_prepare_string(struct ct_string *string)
{
  FT_Face face;
  FT_Error error;
  FT_GlyphSlot slot;
  FT_Bool use_kerning;
  int row, asc;

  /* Load face */
  error = FT_New_Face(library, datafilename_required(string->font), 0, &face);

  if (error == FT_Err_Unknown_File_Format) {
    printf("unknown font format\n");
    assert(0);
  } else if (error) {
    printf("error:%d while opening '%s'\n", error,string->font);
    assert(0);
  }

  slot = face->glyph;
  use_kerning = FT_HAS_KERNING(face);

  /* Set size */
  error = FT_Set_Pixel_Sizes(face, 0, string->font_size);
  assert(!error);

  asc = get_asc(face);

  for (row = 0; row < string->rows; row++) {
    FT_UInt previous = 0;
    struct tr_string_data *result = fc_malloc(sizeof(*result));
    struct ct_point pen = { 0, 0 };
    int i;
    char *text = string->row[row].text;

    string->row[row].data = result;

    /* Alloc memory */
    result->num_glyphs = strlen(text);
    result->glyphs=NULL;
    if (result->num_glyphs > 0) {
      result->glyphs =
	  fc_malloc(sizeof(*result->glyphs) * result->num_glyphs);

      /* Go over all chars */
      for (i = 0; i < result->num_glyphs; i++) {
	unsigned char c = text[i];
	struct ct_point real_pos;
	FT_UInt glyph_index;

	glyph_index = FT_Get_Char_Index(face, c);
	if (glyph_index == 0) {
	  freelog(LOG_VERBOSE, "can't find glyph for %d '%c'", c, c);
	}

	if (use_kerning && previous && glyph_index) {
	  FT_Vector delta;

	  FT_Get_Kerning(face, previous, glyph_index,
			 ft_kerning_default, &delta);
	  pen.x += delta.x >> 6;
	  if (0 && delta.x)
	    freelog(LOG_VERBOSE, "kerning between %c and %c is %ld in '%s'\n",
		    text[i - 1], c, delta.x >> 6, text);
	}

	error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
	assert(!error);

	error =
	    FT_Render_Glyph(slot,
			    string->
			    anti_alias ? ft_render_mode_normal :
			    ft_render_mode_mono);
	if (error) {
	  freelog(LOG_ERROR, "can't render glyph for %d '%c': error=0x%x", c,
		  c, error);
//	  assert(0);
	}

	if (0)
	  printf
	      ("rendered glyph '%c' of size %d:  %dx%d adv=(%ld,%ld) asc=%d desc=%d\n",
	       c, string->font_size, slot->bitmap.width, slot->bitmap.rows,
	       slot->advance.x >> 6, slot->advance.y >> 6,
	       slot->bitmap_top, slot->bitmap.rows - slot->bitmap_top);

	real_pos = pen;
	real_pos.x += slot->bitmap_left;
	real_pos.y += (asc - slot->bitmap_top);

	result->glyphs[i].bitmap = clone_bitmap(&slot->bitmap);
	result->glyphs[i].offset = real_pos;

	pen.x += slot->advance.x >> 6;
	pen.y += slot->advance.y >> 6;

	//pen.x+=string->outline_width/2;
	previous = glyph_index;
      }
    }
    result->size.width = pen.x;
    result->size.height = get_height(face);
  }

  error = FT_Done_Face(face);
  assert(!error);
}

/*************************************************************************
  ...
*************************************************************************/
void tr_free_string(struct ct_string *string)
{
  int row,i;

  for (row = 0; row < string->rows; row++) {
    struct tr_string_data *data = string->row[row].data;

    for (i = 0; i < data->num_glyphs; i++) {
      if (data->glyphs[i].bitmap) {
	free_bitmap(data->glyphs[i].bitmap);
      }
    }
    free(data->glyphs);
    free(data);
  }
}

/*************************************************************************
  ...
*************************************************************************/
void tr_string_get_size(struct ct_size *size, const struct ct_string *string)
{
  int row;

  size->width = 0;
  size->height = 0;
  for (row = 0; row < string->rows; row++) {
    size->width = MAX(size->width, string->row[row].data->size.width);
    size->height = size->height + string->row[row].data->size.height;
  }
}

/*************************************************************************
  ...
*************************************************************************/
void tr_draw_string(struct osda *target,
		    const struct ct_point *position,
		    const struct ct_string *string)
{
  int row, offset_y = 0;

  for (row = 0; row < string->rows; row++) {
    int i, d;
    struct tr_string_data *data = string->row[row].data;


    for (i = 0; i < data->num_glyphs; i++) {
      if (data->glyphs[i].bitmap) {
	struct ct_point p = *position;

	p.x += data->glyphs[i].offset.x;
	p.y += data->glyphs[i].offset.y;
	p.y += offset_y;

	if (string->outline_width > 0) {
	  int w = string->outline_width;

	  assert(w > 0 && w <= 6);

	  for (d = 0; d < HALO_N[w]; d++) {
	    struct ct_point p2 = p;

	    p2.x += HALO_DX[w][d];
	    p2.y += HALO_DY[w][d];
	    be_draw_bitmap(target, string->outline_color, &p2,
			   data->glyphs[i].bitmap);
	  }
	}

	be_draw_bitmap(target, string->foreground, &p,
		       data->glyphs[i].bitmap);
      }
    }
    offset_y += data->size.height;
  }
}
