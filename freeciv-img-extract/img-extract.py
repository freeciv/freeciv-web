#!/usr/bin/python
# -*- coding: iso-8859-1 -*-
''' 
 Freeciv - Copyright (C) 2009,2011 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
'''

import os, sys

import ConfigParser
import Image
import ImageFont, ImageDraw, ImageOps
import json

gfxdir = "";

files = ["../freeciv/freeciv/data/amplio2.tilespec",
  "../freeciv/freeciv/data/amplio2/ancientcities.spec",
  "../freeciv/freeciv/data/amplio2/explosions.spec",
  "../freeciv/freeciv/data/amplio2/fog.spec",
  "../freeciv/freeciv/data/amplio2/grid.spec",
#  "../freeciv/freeciv/data/amplio2/icons.spec",
  "../freeciv/freeciv/data/amplio2/medievalcities.spec",
  "../freeciv/freeciv/data/amplio2/moderncities.spec",
  "../freeciv/freeciv/data/amplio2/mountains.spec",
  "../freeciv/freeciv/data/amplio2/hills.spec",
#  "../freeciv/freeciv/data/amplio2/nuke.spec", #not in use yet
  "../freeciv/freeciv/data/amplio2/ocean.spec",
  "../freeciv/freeciv/data/amplio2/select.spec",
  "../freeciv/freeciv/data/amplio2/terrain1.spec",
  "../freeciv/freeciv/data/amplio2/terrain2.spec",
  "../freeciv/freeciv/data/amplio2/tiles.spec",
  "../freeciv/freeciv/data/amplio2/units.spec",
  "../freeciv/freeciv/data/amplio2/veterancy.spec",
  "../freeciv/freeciv/data/amplio2/water.spec",
  "../freeciv/freeciv/data/misc/wonders-large.spec",
  "../freeciv/freeciv/data/misc/colors.tilespec",
  "../freeciv/freeciv/data/misc/buildings-large.spec",
#  "../freeciv/freeciv/data/misc/icons.spec",
#  "../freeciv/freeciv/data/misc/chiefs.spec",  #not in use yet
  "../freeciv/freeciv/data/misc/overlays.spec",
#  "../freeciv/freeciv/data/misc/citybar.spec",  #not in use yet
  "../freeciv/freeciv/data/misc/shields.spec",
  "../freeciv/freeciv/data/misc/small.spec",
#  "../freeciv/freeciv/data/misc/cursors.spec",
  "../freeciv/freeciv/data/misc/space.spec",
  "../freeciv/freeciv/data/misc/editor.spec",
  "../freeciv/freeciv/data/misc/techs.spec",
  "../freeciv/freeciv/data/misc/flags.spec",
  "../freeciv/freeciv/data/misc/treaty.spec"]; 

global tileset;
global curr_x;
global curr_y;
global max_row_height;
global max_width;
global max_height;
global tileset_inc;

coords = {};
max_width = 0;
max_height = 0;
sum_area = 0;
max_row_height = 0;
curr_x = 0;
curr_y = 14;
# Set size of tileset image manually depending on number of tiles.
# Note!  Safari on iPhone doesn't support more than 3000000 pixels in a single image.
tileset_height = 1000;
tileset_width = 2700;

dither_types = ["t.l0.desert1", "t.l0.plains1", "t.l0.grassland1", "t.l0.forest1", "t.l0.jungle1", "t.l0.hills1", "t.l0.mountains1", "t.l0.tundra1", "t.l0.swamp1"];

tileset = Image.new('RGBA', (tileset_width, tileset_height), (0, 0, 0, 0));
mask_image = Image.open("mask.png");
dither_mask = Image.open("dither.png");
dither_map = {};
tileset_inc = 0;

def increment_tileset_image():
  # save current tileset file.
  global tileset;
  global curr_x;
  global curr_y;
  global max_row_height;
  global max_width;
  global max_height;
  global tileset_inc;

  draw = ImageDraw.Draw(tileset)
  draw.text((130, 0), "Freeciv-web -   GPL Licensed  - Copyright 2007-2013  Andreas Rosdal", fill="rgb(0,0,0)")

  tileset.save("pre-freeciv-web-tileset-" + str(tileset_inc) + ".png");
  tileset_inc += 1;

  tileset = Image.new('RGBA', (tileset_width, tileset_height), (0, 0, 0, 0));

  curr_x = 0;
  curr_y = 16;
  max_row_height = 0;
  max_width = 0;
  max_height = 0;

for file in files:
  config = ConfigParser.ConfigParser()
  print("Parsing " + file );
  config.read(file);
  print(config.sections());

  if "extra" in config.sections():
    csprites = config.get("extra", "sprites");
    sprites = csprites.replace("{","").replace("}", "").replace(" ", "").replace("\"","").split("\n");
    for sprite in sprites:
      rsprite = sprite.split(",");
      if (len(rsprite) != 2 or rsprite[1] == "file"): continue;

      tag = rsprite[1];
      if (tag.find(";")): tag = tag.split(";")[0]; 
      tag = tag.strip();

      im = Image.open(gfxdir + tag + ".png");
      (w, h) = im.size;
      if (w > max_width): max_width = w;
      if (h > max_height): max_height = h;
      sum_area += (h*w);

      if (curr_y + h >= tileset_height):
        increment_tileset_image();
      tileset.paste(im, (curr_x, curr_y));
      coords[rsprite[0]] = (curr_x, curr_y, w, h, tileset_inc);
      curr_x += w;
      if (h > max_row_height): max_row_height = h;
      if (w + curr_x >= tileset_width): 
        curr_x = 0;
        curr_y += max_row_height;
        max_row_height = 0;


      #im.save(rsprite[0] + ".png");
      

  if "file" in config.sections():
    gfx = config.get("file", "gfx") + ".png";
    print(gfx);
    im = Image.open(gfxdir + gfx.replace("\"", ""));

    if "grid_main" in config.sections():
      dx = int(config.get("grid_main", "dx"));
      dy = int(config.get("grid_main", "dy"));
      x_top_left = int(config.get("grid_main", "x_top_left"));
      y_top_left = int(config.get("grid_main", "y_top_left"));
      pixel_border = 0;
      try:
        pixel_border = int(config.get("grid_main", "pixel_border"));
      except:
        pass;

      print("pixel_border " + str(pixel_border));
      tag2 = None;

      tiles_buf = config.get("grid_main", "tiles");
      tiles = tiles_buf.replace("{","").replace("}", "").replace(" ", "").replace("\"","").split("\n");
      for tile in tiles:
        tmptile = tile.split(",");
        if (tmptile[0] == "row" or tmptile[0] == "" or len(tmptile) < 3) : continue;
        gx = int(tmptile[1]);
        gy = int(tmptile[0]);
        tag = str(tmptile[2]);

        if (tag.find(";")): tag = tag.split(";")[0]; 
        tag = tag.strip();
          
        if len(tmptile) == 4: 
          tag2 = tmptile[3];
          if (tag2.find(";")): 
            tag2 = tag2.split(";")[0]; 
            tag2 = tag2.strip();

        dims = (x_top_left + pixel_border*gx + gx*dx, 
                                      y_top_left + pixel_border*gy + gy*dy, 
                                      x_top_left + gx*dx + dx + pixel_border*gx, 
                                      y_top_left + gy*dy + dy + pixel_border*gy)

        result_tile = im.copy().crop(dims); 
        #result_tile.save(tag + ".png");

        #if (tag == "t.l0.grassland1"):
        #  result_tile = Image.open("grassland1.png");
        #if (tag == "t.l0.plains1"):
        #  result_tile = Image.open("plains1.png");

        if (tag.find("cellgroup") != -1):
          #handle a cell group (1 tile = 4 cells)
          (WZ, HZ) = result_tile.size;
	  xf = [WZ / 4, WZ / 4, 0, WZ / 2];
	  yf = [HZ / 2, 0, HZ / 4, HZ / 4];
	  xo = [0, 0, -WZ / 2, WZ / 2];
	  yo = [HZ / 2, -HZ / 2, 0, 0];

          for dir in range(4):
            result_cell = result_tile.copy().crop((xf[dir], yf[dir], xf[dir] + (WZ / 2), yf[dir] + (HZ / 2)));
            mask = Image.new('RGBA', (WZ/2, HZ/2), (255));
            mask.paste(mask_image, (xo[dir] - xf[dir], yo[dir] - yf[dir]));
           
            (w, h) = result_cell.size;
            if (w > max_width): max_width = w;
            if (h > max_height): max_height = h;
            sum_area += (h*w);

            if (curr_y + h >= tileset_height):
              increment_tileset_image();

            tileset.paste(result_cell, (curr_x, curr_y), mask);
            store_img = Image.new('RGBA', (w, h), (0, 0, 0, 0));
            store_img.paste(result_cell, (0, 0), mask);
            coords[tag + "." + str(dir)] = (curr_x, curr_y, w, h, tileset_inc);
            curr_x += w;
            if (h > max_row_height): max_row_height = h;
            if (w + curr_x >= tileset_width): 
              curr_x = 0;
              curr_y += max_row_height;
              max_row_height = 0;

	elif tag in dither_types:
          # handle a dithered tile.
          dither_map[tag] = result_tile.copy();

        else:
          # handle a non-cellgroup tile.
          (w, h) = result_tile.size;
          if (w > max_width): max_width = w;
          if (h > max_height): max_height = h;
          sum_area += (h*w);

          if (curr_y + h >= tileset_height):
            increment_tileset_image();

          tileset.paste(result_tile, (curr_x, curr_y));

          coords[tag] = (curr_x, curr_y, w, h, tileset_inc);
          curr_x += w;
          if (h > max_row_height): max_row_height = h;
          if (w + curr_x >= tileset_width): 
            curr_x = 0;
            curr_y += max_row_height;
            max_row_height = 0;

          #if (tag2 != None): result_tile.save(tag2 + ".png"); 
          if (tag2 != None and len(tag2) > 0): 
            coords[tag2] = (curr_x, curr_y, w, h, tileset_inc);
	    print("saving tag: " + tag2);
 
    if "grid_coasts" in config.sections():
      dx = int(config.get("grid_coasts", "dx"));
      dy = int(config.get("grid_coasts", "dy"));
      x_top_left = int(config.get("grid_coasts", "x_top_left"));
      y_top_left = int(config.get("grid_coasts", "y_top_left"));
      pixel_border = 0;
      try:
        pixel_border = int(config.get("grid_coasts", "pixel_border"));
      except:
        pass;

      print("pixel_border " + str(pixel_border));
      tag2 = None;

      tiles_buf = config.get("grid_coasts", "tiles");
      tiles = tiles_buf.replace("{","").replace("}", "").replace(" ", "").replace("\"","").split("\n");
      for tile in tiles:
        tmptile = tile.split(",");
        if (tmptile[0] == "row"): continue;
	if (tmptile[0] == ""): continue;
        gx = int(tmptile[1]);
        gy = int(tmptile[0]);
        tag = str(tmptile[2]);

        if (tag.find(";")): tag = tag.split(";")[0]; 
        tag = tag.strip();
          
        if len(tmptile) == 4: 
          tag2 = tmptile[3];
          if (tag2.find(";")): 
            tag2 = tag2.split(";")[0]; 
            tag2 = tag2.strip();

        dims = (x_top_left + pixel_border*gx + gx*dx, 
                                      y_top_left + pixel_border*gy + gy*dy, 
                                      x_top_left + gx*dx + dx + pixel_border*gx, 
                                      y_top_left + gy*dy + dy + pixel_border*gy)

        result_tile = im.copy().crop(dims); 
        #result_tile.save(tag + ".png");
        (w, h) = result_tile.size;
        if (w > max_width): max_width = w;
        if (h > max_height): max_height = h;
        sum_area += (h*w);

        if (curr_y + h >= tileset_height):
          increment_tileset_image();

        tileset.paste(result_tile, (curr_x, curr_y));

        coords[tag] = (curr_x, curr_y, w, h, tileset_inc);
        curr_x += w;
        if (h > max_row_height): max_row_height = h;
        if (w + curr_x >= tileset_width): 
          curr_x = 0;
          curr_y += max_row_height;
          max_row_height = 0;

        #if (tag2 != None): result_tile.save(tag2 + ".png"); 
        if (tag2 != None): 
          coords[tag2] = (curr_x, curr_y, w, h, tileset_inc);


# handle dithered tiles.
for src_key in dither_map.keys():
  for alt_key in dither_map.keys():
      for dir in range(4):
	tag = str(dir) + src_key[5:].replace("1", "") + "_" + alt_key[5:].replace("1", "");
	print(tag);
        src_img = dither_map[src_key].copy();
        alt_img = dither_map[alt_key].copy();
        src_img.paste(alt_img, None, dither_mask);

        (WZ, HZ) = src_img.size;
        xf = [WZ / 2, 0, WZ / 2, 0];
        yf = [0, HZ / 2, HZ / 2, 0];

        result_cell = src_img.crop((xf[dir], yf[dir], xf[dir] + (WZ / 2), yf[dir] + (HZ / 2)));

        (w, h) = (WZ / 2 , HZ / 2);
        if (w > max_width): max_width = w;
        if (h > max_height): max_height = h;
        sum_area += (h*w);
        if (curr_y + h >= tileset_height):
          increment_tileset_image();
        tileset.paste(result_cell, (curr_x, curr_y));

        coords[tag] = (curr_x, curr_y, w, h, tileset_inc);

        curr_x += w;
        if (h > max_row_height): max_row_height = h;
        if (w + curr_x >= tileset_width): 
          curr_x = 0;
          curr_y += max_row_height;
          max_row_height = 0;

im = Image.open("city_active.png");
(w, h) = im.size;
curr_x += w;
tileset.paste(im, (curr_x, curr_y));
coords["city_active"] = (curr_x, curr_y, w, h, tileset_inc);

#FIXME: the city_active and city_invalid graphics must not be drawn outside
# of the full final tileset image.
im = Image.open("city_invalid.png");
(w, h) = im.size;
curr_x += w;
tileset.paste(im, (curr_x, curr_y));
coords["city_invalid"] = (curr_x, curr_y, w, h, tileset_inc);


increment_tileset_image();

print("MAX: " + str(max_width) + "  " + str(max_height) + "  " + str(sum_area));

f = open('freeciv-web-tileset.js', 'w')

f.write("var tileset = " + json.dumps(coords, separators=(',',':')));


print("done.");
