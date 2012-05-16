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

#ifndef FC__BE_COMMON_CAIRO32_H
#define FC__BE_COMMON_CAIRO32_H

#include <cairo/cairo.h>

#include "common_types.h"

#define ENABLE_IMAGE_ACCESS_CHECK	0

/* Internal 32bpp format. */
struct image {
  int width, height;
  int pitch;
  cairo_surface_t *data;
  struct ct_rect full_rect;
};

struct mem_image;

cairo_surface_t *cairo_image_surface_create_from_mem_image(const struct mem_image *image);
cairo_surface_t *be_cairo_surface_create(int width, int height);

#endif /* FC__BE_COMMON_CAIRO32_H */
