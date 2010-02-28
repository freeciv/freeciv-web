/**********************************************************************
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h>

#include <cairo/cairo.h>

#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "support.h"

#include "back_end.h"
#include "be_common_pixels.h"

#include "be_common_cairo_32.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#define ENABLE_TRANSPARENCY	1
#define DO_CYCLE_MEASUREMENTS	0

/* Internal 32bpp format. Used temporarily. */
struct mem_image {
  int width, height;
  int pitch;
  unsigned char *data;
  struct ct_rect full_rect;
};

struct mem_image *mem_image_create(int width, int height);

#define IMAGE_GET_ADDRESS(image, x, y) ((image)->data + (image)->pitch * (y) + 4 * (x))

#if ENABLE_IMAGE_ACCESS_CHECK
#define IMAGE_CHECK(image, x, y) \
if((image)->data==NULL || (x)<0 || (x)>=(image)->width || (y)<0 || (y)>=(image)->height) { \
    printf("ERROR: image data=%p size=(%dx%d) pos=(%d,%d)\n",\
	   (image)->data, (image)->width,(image)->height,(x),(y));\
    assert(0);\
}
#else
#define IMAGE_CHECK(image, x, y)
#endif

/* Blend the RGB values of two pixel 3-byte pixel arrays
 * based on a source alpha value. */
#define ALPHA_BLEND(d, A, s)          \
do {                                  \
  d[0] = (((s[0]-d[0])*(A))>>8)+d[0]; \
  d[1] = (((s[1]-d[1])*(A))>>8)+d[1]; \
  d[2] = (((s[2]-d[2])*(A))>>8)+d[2]; \
} while(0)

/*************************************************************************
  Create cairo image surface from image struct
*************************************************************************/
cairo_surface_t *cairo_image_surface_create_from_mem_image(
                                                 const struct mem_image *image) {
													 
  return cairo_image_surface_create_for_data(image->data, CAIRO_FORMAT_ARGB32,
                                   image->width, image->height, image->pitch);
}

/*************************************************************************
  Combine RGB colour values into a 24bpp colour value.
*************************************************************************/
be_color be_get_color(int red, int green, int blue, int alpha)
{
  assert(red >= 0 && red <= 255);
  assert(green >= 0 && green <= 255);
  assert(blue >= 0 && blue <= 255);
  assert(alpha >= 0 && alpha <= 255);

  return red << 24 | green << 16 | blue << 8 | alpha;
}

/*************************************************************************
  Set a pixel in a given buffer (dest) to a given color (color).
*************************************************************************/
static void set_color(be_color color, unsigned char *dest)
{
  /* RGBA color variable -> ARGB format in little endian buffer = BGRA */
  dest[0] = ((color >> 8) & 0xff);  /* blue */	
  dest[1] = ((color >> 16) & 0xff); /* green */	
  dest[2] = ((color >> 24) & 0xff); /* red */
  dest[3] = ((color) & 0xff);       /* alpha */
}

/*************************************************************************
  Draw an empty rectangle in given osda with given drawing type.
*************************************************************************/
void be_draw_rectangle(struct osda *target, const struct ct_rect *spec,
		       int line_width, be_color color)
{
  cairo_t *cr;
  unsigned char color_tmp[4];
  

  set_color(color, color_tmp);
	

  cr = cairo_create(target->image->data);

  cairo_rectangle(cr, 0.0, 0.0, target->image->width, target->image->height);
  cairo_clip(cr);
  
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
  cairo_set_line_width(cr, line_width);
  cairo_set_source_rgba(cr, (double)color_tmp[2]/255, (double)color_tmp[1]/255,
                            (double)color_tmp[0]/255, (double)color_tmp[3]/255);
  cairo_rectangle(cr, spec->x, spec->y, spec->width, spec->height);
	
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_stroke(cr);
	
  cairo_destroy(cr);
}

/*************************************************************************
  Draw a line in given osda with given drawing type.
*************************************************************************/
void be_draw_line(struct osda *target,
		  const struct ct_point *start,
		  const struct ct_point *end,
		  int line_width, bool dashed, be_color color)
{
  cairo_t *cr;
  unsigned char color_tmp[4];

  set_color(color, color_tmp);
	
  cr = cairo_create(target->image->data);

  cairo_rectangle(cr, 0.0, 0.0, target->image->width, target->image->height);
  cairo_clip(cr);
	
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
  cairo_set_line_width(cr, line_width);
	
  if (dashed) {
    double dashes = 3;
    cairo_set_dash(cr, &dashes, 1, 0.0);
  }
	
  cairo_set_source_rgba(cr, (double)color_tmp[2]/255, (double)color_tmp[1]/255,
                            (double)color_tmp[0]/255, (double)color_tmp[3]/255);
  cairo_move_to(cr, start->x, start->y);
  cairo_line_to(cr, end->x, end->y);
	
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_stroke(cr);
	
  cairo_destroy(cr);
}

/*************************************************************************
  Fill a square region in given osda with given colour and drawing type.
*************************************************************************/
void be_draw_region(struct osda *target, 
		    const struct ct_rect *region, be_color color)
{
  cairo_t *cr;
  unsigned char color_tmp[4];

  set_color(color, color_tmp);
	
  cr = cairo_create(target->image->data);

  cairo_rectangle(cr, 0.0, 0.0, target->image->width, target->image->height);
  cairo_clip(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);	
  cairo_set_source_rgba(cr, (double)color_tmp[2]/255, (double)color_tmp[1]/255,
                            (double)color_tmp[0]/255, (double)color_tmp[3]/255);
  cairo_rectangle(cr, region->x, region->y, region->width, region->height);
  cairo_fill(cr);
	
  cairo_destroy(cr);
}

/*************************************************************************
  Return TRUE iff pixel in given osda is transparent or out of bounds.
*************************************************************************/
bool be_is_transparent_pixel(struct osda *osda, const struct ct_point *pos)
{
  cairo_t *cr;
  cairo_surface_t *dest_surface;
  
  struct ct_rect bounds = { 0, 0, osda->image->width, osda->image->height };
  if (!ct_point_in_rect(pos, &bounds)) {
    return FALSE;
  }

  struct mem_image *tmp_image = mem_image_create(1, 1);
    
  dest_surface = cairo_image_surface_create_from_mem_image(tmp_image);
  
  cr = cairo_create(dest_surface);
  
  cairo_rectangle(cr, 0, 0, 1, 1);
  cairo_clip(cr);
  
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface(cr, osda->image->data, -pos->x, -pos->y);
  cairo_paint(cr);
  
  cairo_destroy(cr);
  cairo_surface_destroy(dest_surface);
  
  IMAGE_CHECK(tmp_image, 0, 0);
  return IMAGE_GET_ADDRESS(tmp_image, 0, 0)[3] != MAX_OPACITY;
}

/*************************************************************************
  Image blit with transparency.  Alpha value lifted from src.
  MIN_OPACITY means the pixel should not be drawn.  MAX_OPACITY means
  the pixels should not be blended.
*************************************************************************/
/* non static to prevent inlining */
void image_blit_masked_trans(const struct ct_size *size,
			     const struct image *src,
			     const struct ct_point *src_pos,
			     struct image *dest,
			     const struct ct_point *dest_pos);
void image_blit_masked_trans(const struct ct_size *size,
			     const struct image *src,
			     const struct ct_point *src_pos,
			     struct image *dest,
			     const struct ct_point *dest_pos)
{
  cairo_t *cr;

  cr = cairo_create(dest->data);

  cairo_rectangle(cr, dest_pos->x, dest_pos->y, size->width, size->height);
  cairo_clip(cr);

  cairo_translate(cr, dest_pos->x, dest_pos->y);
  
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_surface(cr, src->data, -src_pos->x, -src_pos->y);
  cairo_paint(cr);
	
  cairo_destroy(cr);
}

/*************************************************************************
  ...
*************************************************************************/
static void clip_two_regions(const struct image *dest,
			     const struct image *src,
			     struct ct_size *size,
			     struct ct_point *dest_pos,
			     struct ct_point *src_pos)
{
  int dx, dy;
  struct ct_point real_src_pos, real_dest_pos;
  struct ct_size real_size;

  struct ct_rect src_use =
      { src_pos->x, src_pos->y, size->width, size->height };
  struct ct_rect dest_use =
      { dest_pos->x, dest_pos->y, size->width, size->height };

  real_src_pos = *src_pos;
  real_dest_pos = *dest_pos;

  ct_clip_rect(&src_use, &src->full_rect);
  ct_clip_rect(&dest_use, &dest->full_rect);

  real_size.width = MIN(src_use.width, dest_use.width);
  real_size.height = MIN(src_use.height, dest_use.height);

  dx = MAX(src_use.x - src_pos->x, dest_use.x - dest_pos->x);
  dy = MAX(src_use.y - src_pos->y, dest_use.y - dest_pos->y);

  real_src_pos.x += dx;
  real_src_pos.y += dy;

  real_dest_pos.x += dx;
  real_dest_pos.y += dy;

  *size=real_size;
  *src_pos=real_src_pos;
  *dest_pos=real_dest_pos;
}

/*************************************************************************
  Blit one image onto another, using transparency value given on all
  pixels that have alpha mask set.
*************************************************************************/
void image_copy(struct image *dest, struct image *src,
                const struct ct_size *size, const struct ct_point *dest_pos,
                const struct ct_point *src_pos)
{
  struct ct_point real_src_pos = *src_pos, real_dest_pos = *dest_pos;
  struct ct_size real_size = *size;

  clip_two_regions(dest, src, &real_size, &real_dest_pos, &real_src_pos);

  if (DO_CYCLE_MEASUREMENTS) {
#define rdtscll(val) __asm__ __volatile__ ("rdtsc" : "=A" (val))
    unsigned long long start, end;
    static unsigned long long total_clocks = 0, total_pixels = 0;
    static int total_blits = 0;

    rdtscll(start);

    image_blit_masked_trans(&real_size, src, &real_src_pos, dest,
			    &real_dest_pos);

    rdtscll(end);
    printf("BLITTING %dx%d %dx%d->%dx%d %lld %d\n", real_size.width,
	   real_size.height, src->width, src->height, dest->width, dest->height,
	   end - start, real_size.width * real_size.height);
    total_clocks += (end - start);
    total_pixels += (real_size.width * real_size.height);
    total_blits++;
    if ((total_blits % 1000) == 0) {
      printf("%f cycles per pixel\n", (float) total_clocks / total_pixels);
    }
  } else {
    image_blit_masked_trans(&real_size, src, &real_src_pos, dest,
			    &real_dest_pos);
  }
}

/*************************************************************************
  See be_draw_bitmap() below.
*************************************************************************/
void draw_mono_bitmap(struct image *image, be_color color,
                      const struct ct_point *position, 
                      struct FT_Bitmap_ *bitmap)
{
  int x, y;
  unsigned char *pbitmap = (unsigned char *) bitmap->buffer;
  unsigned char tmp[4];
  cairo_t *cr;
  cairo_surface_t *src_surface;

  struct mem_image *tmp_image = mem_image_create(bitmap->width, bitmap->rows);
  
  set_color(color, tmp);

  for (y = 0; y < bitmap->rows; y++) {
    for (x = 0; x < bitmap->width; x++) {
      unsigned char bv = bitmap->buffer[x / 8 + bitmap->pitch * y];
      if (TEST_BIT(bv, 7 - (x % 8))) {
        unsigned char *p = IMAGE_GET_ADDRESS(tmp_image, x, y);
	IMAGE_CHECK(tmp_imageimage, x, y);
        memcpy(p, tmp, 4);
      }
    }
    pbitmap += bitmap->pitch;
  }

  src_surface = cairo_image_surface_create_for_data(tmp_image->data,
                                 CAIRO_FORMAT_ARGB32, 
                                 tmp_image->width, tmp_image->height, tmp_image->pitch);
  
  cr = cairo_create(image->data);
  
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_surface(cr, src_surface, position->x, position->y);
  cairo_paint(cr);
	
  cairo_destroy(cr);
  cairo_surface_destroy(src_surface);
}

/*************************************************************************
  Draw the given bitmap (ie a 1bpp pixmap) in the given color and 
  position.
*************************************************************************/
void draw_alpha_bitmap(struct image *image, be_color color_,
                       const struct ct_point *position,
                       struct FT_Bitmap_ *bitmap)
{
  int x, y;
  unsigned char color[4];
  unsigned char *pbitmap = (unsigned char *) bitmap->buffer;
  cairo_t *cr;
  cairo_surface_t *src_surface;

  struct mem_image *tmp_image = mem_image_create(bitmap->width, bitmap->rows);

  set_color(color_, color);

  for (y = 0; y < bitmap->rows; y++) {
    for (x = 0; x < bitmap->width; x++) {
      unsigned short transparency = pbitmap[x];
      unsigned char tmp[4];
      unsigned char *p = IMAGE_GET_ADDRESS(tmp_image, x, y);
      IMAGE_CHECK(tmp_image, x, y);
      ALPHA_BLEND(tmp, transparency, p);
      memcpy(p, tmp, 4);
    }
    pbitmap += bitmap->pitch;
  }

  src_surface = cairo_image_surface_create_for_data(tmp_image->data,
                                 CAIRO_FORMAT_ARGB32, 
                                 tmp_image->width, tmp_image->height, tmp_image->pitch);

  cr = cairo_create(image->data);
  
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_surface(cr, src_surface, -position->x, -position->y);
  cairo_paint(cr);
	
  cairo_destroy(cr);
  cairo_surface_destroy(src_surface);
}

/*************************************************************************
  Set the alpha mask of pixels in a given region of an image.
*************************************************************************/
void image_set_alpha(const struct image *image, const struct ct_rect *rect,
		     unsigned char alpha)
{
  cairo_t *cr;
	
  cr = cairo_create(image->data);

  cairo_rectangle(cr, 0.0, 0.0, rect->width, rect->height);
  cairo_clip(cr);

  cairo_translate(cr, rect->x, rect->y);
  
  cairo_paint_with_alpha(cr, alpha/255);
	
  cairo_destroy(cr);
}

/*************************************************************************
  Perform 
     dest_alpha = (dest_alpha * src_alpha)/256
*************************************************************************/
void image_multiply_alphas(struct image *dest, const struct image *src,
			   const struct ct_point *src_pos)
{
  cairo_t *cr;

  cr = cairo_create(dest->data);

  cairo_set_operator(cr, CAIRO_OPERATOR_DEST_IN);
  cairo_set_source_surface(cr, src->data, -src_pos->x, -src_pos->y);
  cairo_paint(cr);
	
  cairo_destroy(cr);
}

/*************************************************************************
  Copy image buffer src to dest without doing any alpha-blending.
*************************************************************************/
void image_copy_full(struct image *src, struct image *dest,
		     struct ct_rect *region)
{
  cairo_t *cr;
	
  cr = cairo_create(dest->data);
  
  cairo_rectangle(cr, 0, 0, region->width, region->height);
  cairo_clip(cr);	
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface(cr, src->data, -region->x, -region->y);	
  cairo_paint(cr);
	
  cairo_destroy(cr);
}

/*************************************************************************
  Allocate and initialize an image struct.
*************************************************************************/
struct image *image_create(int width, int height)
{
  struct image *result = fc_malloc(sizeof(*result));

  result->pitch = width * 4;
  result->data = be_cairo_surface_create(width, height);
  result->width = width;
  result->height = height;
  result->full_rect.x = 0;
  result->full_rect.y = 0;
  result->full_rect.width = width;
  result->full_rect.height = height;

  return result;
}

struct mem_image *mem_image_create(int width, int height)
{
  struct mem_image *result = fc_malloc(sizeof(*result));

  result->pitch = width * 4;
  result->data = fc_calloc(result->pitch * height, 1);
  result->width = width;
  result->height = height;
  result->full_rect.x = 0;
  result->full_rect.y = 0;
  result->full_rect.width = width;
  result->full_rect.height = height;

  return result;
}

/*************************************************************************
  Free an image struct.
*************************************************************************/
void image_destroy(struct image *image)
{
  free(image->data);
  free(image);
}

/*************************************************************************
  Get image properties
*************************************************************************/
int image_get_width(struct image *image) {
  return image->width;
}

int image_get_height(struct image *image) {
  return image->height;
}

struct ct_rect *image_get_full_rect(struct image *image) {
  return &image->full_rect;	
}

/*************************************************************************
  ...
*************************************************************************/
struct image *image_load_gfxfile(const char *filename)
{
  struct image *xi;		
  cairo_surface_t *png_surface;
  cairo_t *cr;
  int width, height;
  
  png_surface = cairo_image_surface_create_from_png(filename);
  
  if (cairo_surface_status(png_surface) != CAIRO_STATUS_SUCCESS) {
	freelog(LOG_FATAL, "Failed reading graphics file: %s", filename);
	freelog(LOG_FATAL, "  %s", cairo_status_to_string(
                               cairo_surface_status(png_surface)));	  
  }
  
  width = cairo_image_surface_get_width(png_surface);
  height = cairo_image_surface_get_height(png_surface);
  
  xi = image_create(width, height);

  cr = cairo_create(xi->data);
  
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface(cr, png_surface, 0.0, 0.0);
  cairo_paint(cr);
  
  cairo_destroy(cr);
  
  cairo_surface_destroy(png_surface);
  
  return xi;
}

/*************************************************************************
  Write an image buffer to file.
*************************************************************************/
void be_write_osda_to_file(struct osda *osda, const char *filename)
{
  const char *fileext = ".png";
  char *real_filename = fc_malloc(strlen(filename) + strlen(fileext));

  mystrlcpy(real_filename, filename, strlen(filename));
  mystrlcat(real_filename, fileext, strlen(fileext));
	
  cairo_surface_write_to_png(osda->image->data, real_filename);
	
  free(real_filename);
}
