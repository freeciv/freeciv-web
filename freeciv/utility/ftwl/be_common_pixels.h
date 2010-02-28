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

#ifndef FC__BE_COMMON_PIXELS_H
#define FC__BE_COMMON_PIXELS_H

struct image;

struct osda {
  int magic;
  struct image *image;
  bool has_transparent_pixels;
};

struct sprite {
  struct image *image;
};

void draw_mono_bitmap(struct image *image, be_color color,
                      const struct ct_point *position,
                      struct FT_Bitmap_ *bitmap);
void draw_alpha_bitmap(struct image *image, be_color color_,
                       const struct ct_point *position,
                       struct FT_Bitmap_ *bitmap);

struct image *image_create(int width, int height);
void image_destroy(struct image *image);

int image_get_width(struct image *image);
int image_get_height(struct image *image);	
struct ct_rect *image_get_full_rect(struct image *image);

struct image *image_clone_sub(struct image *src, const struct ct_point *pos, 
			      const struct ct_size *size);
void image_copy_full(struct image *src, struct image *dest,
		     struct ct_rect *region);
void image_copy(struct image *dest, struct image *src,
                const struct ct_size *size, const struct ct_point *dest_pos,
                const struct ct_point *src_pos);
void image_set_alpha(const struct image *image, const struct ct_rect *rect,
		     unsigned char alpha);
void image_multiply_alphas(struct image *dest, const struct image *src,
                           const struct ct_point *src_pos);
struct image *image_load_gfxfile(const char *filename);

#endif /* FC__BE_COMMON_PIXELS_H */
