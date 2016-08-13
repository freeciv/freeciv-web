#!/usr/bin/python
# -*- coding: iso-8859-1 -*-
''' 
 Freeciv - Copyright (C) 2009-2016 - Andreas Røsdal   andrearo@pvv.ntnu.no
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
os.environ['PYTHON_EGG_CACHE'] = '/tmp'

import configparser
from PIL import Image
from PIL import ImageFont, ImageDraw, ImageOps
import json

gfxdir = "";

files = {"amplio2" : [
  "../../freeciv/freeciv/data/amplio2/terrain1.spec",
  "../../freeciv/freeciv/data/amplio2.tilespec",
  "../../freeciv/freeciv/data/amplio2/activities.spec",
  "../../freeciv/freeciv/data/amplio2/cities.spec",
  "../../freeciv/freeciv/data/amplio2/bases.spec",
  "../../freeciv/freeciv/data/amplio2/explosions.spec",
  "../../freeciv/freeciv/data/amplio2/fog.spec",
  "../../freeciv/freeciv/data/amplio2/grid.spec",
  "../../freeciv/freeciv/data/amplio2/nuke.spec",
  "../../freeciv/freeciv/data/amplio2/mountains.spec",
  "../../freeciv/freeciv/data/amplio2/hills.spec",
  "../../freeciv/freeciv/data/amplio2/ocean.spec",
  "../../freeciv/freeciv/data/amplio2/select.spec",
  "../../freeciv/freeciv/data/amplio2/terrain2.spec",
  "../../freeciv/freeciv/data/amplio2/tiles.spec",
  "../../freeciv/freeciv/data/amplio2/units.spec",
  "../../freeciv/freeciv/data/amplio2/upkeep.spec",
  "../../freeciv/freeciv/data/amplio2/veterancy.spec",
  "../../freeciv/freeciv/data/amplio2/water.spec",
  "../../freeciv/freeciv/data/misc/wonders-large.spec",
  "../../freeciv/freeciv/data/misc/colors.tilespec",
  "../../freeciv/freeciv/data/misc/buildings-large.spec",
  "../../freeciv/freeciv/data/misc/overlays.spec",
  "../../freeciv/freeciv/data/misc/shields.spec",
  "../../freeciv/freeciv/data/misc/small.spec",
  "../../freeciv/freeciv/data/misc/governments.spec",
  "../../freeciv/freeciv/data/misc/specialists.spec",
  "../../freeciv/freeciv/data/misc/space.spec",
  "../../freeciv/freeciv/data/misc/editor.spec",
  "../../freeciv/freeciv/data/misc/techs.spec",
  "../../freeciv/freeciv/data/misc/flags.spec",
  "../../freeciv/freeciv/data/misc/treaty.spec",
  "../../freeciv/freeciv/data/misc/citybar.spec"],
  "trident" : [
  "../../freeciv/freeciv/data/trident/auto_ll.spec",
  "../../freeciv/freeciv/data/trident/fog.spec",
  "../../freeciv/freeciv/data/trident/roads.spec",
  "../../freeciv/freeciv/data/trident/tiles.spec",
  "../../freeciv/freeciv/data/trident/cities.spec",
  "../../freeciv/freeciv/data/trident/explosions.spec",
  "../../freeciv/freeciv/data/trident/grid.spec",
  "../../freeciv/freeciv/data/trident/select.spec",
  "../../freeciv/freeciv/data/trident/units.spec",
  "../../freeciv/freeciv/data/misc/wonders-large.spec",
  "../../freeciv/freeciv/data/misc/colors.tilespec",
  "../../freeciv/freeciv/data/misc/buildings-large.spec",
  "../../freeciv/freeciv/data/misc/overlays.spec",
  "../../freeciv/freeciv/data/misc/shields.spec",
  "../../freeciv/freeciv/data/misc/small.spec",
  "../../freeciv/freeciv/data/misc/governments.spec",
  "../../freeciv/freeciv/data/misc/specialists.spec",
  "../../freeciv/freeciv/data/misc/space.spec",
  "../../freeciv/freeciv/data/misc/editor.spec",
  "../../freeciv/freeciv/data/misc/techs.spec",
  "../../freeciv/freeciv/data/misc/flags.spec",
  "../../freeciv/freeciv/data/misc/treaty.spec",
  "../../freeciv/freeciv/data/misc/citybar.spec"
  ],
  "isotrident" : [
  "../../freeciv/freeciv/data/isotrident/terrain1.spec",
  "../../freeciv/freeciv/data/isotrident/cities.spec",
  "../../freeciv/freeciv/data/isotrident/fog.spec",
  "../../freeciv/freeciv/data/isotrident/grid.spec",
  "../../freeciv/freeciv/data/isotrident/morecities.spec",
  "../../freeciv/freeciv/data/isotrident/nuke.spec",
  "../../freeciv/freeciv/data/isotrident/select.spec",
  "../../freeciv/freeciv/data/isotrident/terrain2.spec",
  "../../freeciv/freeciv/data/isotrident/ocean.spec",
  "../../freeciv/freeciv/data/isotrident/tiles.spec",
  "../../freeciv/freeciv/data/isotrident/unitextras.spec",
  "../../freeciv/freeciv/data/misc/wonders-large.spec",
  "../../freeciv/freeciv/data/misc/colors.tilespec",
  "../../freeciv/freeciv/data/misc/buildings-large.spec",
  "../../freeciv/freeciv/data/misc/overlays.spec",
  "../../freeciv/freeciv/data/misc/shields.spec",
  "../../freeciv/freeciv/data/misc/small.spec",
  "../../freeciv/freeciv/data/misc/governments.spec",
  "../../freeciv/freeciv/data/misc/specialists.spec",
  "../../freeciv/freeciv/data/misc/space.spec",
  "../../freeciv/freeciv/data/misc/editor.spec",
  "../../freeciv/freeciv/data/misc/techs.spec",
  "../../freeciv/freeciv/data/misc/flags.spec",
  "../../freeciv/freeciv/data/misc/treaty.spec",
  "../../freeciv/freeciv/data/misc/citybar.spec"
  ]
}; 

global tileset;
global curr_x;
global curr_y;
global max_row_height;
global max_width;
global max_height;
global tileset_inc;
global dither_map;
global dither_mask;
global mask_image;

coords = {};
max_width = 0;
max_height = 0;
sum_area = 0;
max_row_height = 0;
curr_x = 0;
curr_y = 14;
# Set size of tileset image manually depending on number of tiles.
# Note!  Safari on iPhone doesn't support more than 3000000 pixels in a single image.
tileset_height = 1030;
tileset_width = 1800;

dither_types = ["t.l0.desert1", "t.l0.plains1", "t.l0.grassland1", "t.l0.forest1", "t.l0.jungle1", "t.l0.hills1", "t.l0.mountains1", "t.l0.tundra1", "t.l0.swamp1"];
print("Freeciv-img-extract running with PIL " + Image.VERSION);
tileset = Image.new('RGBA', (tileset_width, tileset_height), (0, 0, 0, 0));
mask_image = None;
dither_mask = None;
dither_map = {};
tileset_inc = 0;

def config_read(file):
  print(("Parsing " + file ));
  config = configparser.ConfigParser(strict=False)
  with open (file, "r") as myfile:
    config_text=myfile.read();
    # These changes are required so that the Python config parser can read Freeciv spec files.
    config_text = config_text.replace("*include", "#*include");
    config_text = config_text.replace("\n\"", "\n \"");
    config_text = config_text.replace("\n}", "\n }");
    config_text = config_text.replace("\n1", "\n 1");
    config_text = config_text.replace("grid_star", "grid_main");
    config_text = config_text.replace("         \"unit.auto_settler\"", "  4, 10, \"unit.auto_settler\"");
    config_text = config_text.replace("\t  ;", "\n;");
    config.read_string(config_text);
    print((config.sections()));
    return config;

def increment_tileset_image(tileset_name):
  # save current tileset file.
  global tileset;
  global curr_x;
  global curr_y;
  global max_row_height;
  global max_width;
  global max_height;
  global tileset_inc;

  draw = ImageDraw.Draw(tileset)
  draw.text((130, 0), "Freeciv-web - http://play.freeciv.org/  GPL Licensed  - Copyright 2007-2015  Andreas Rosdal", fill="rgb(0,0,0)")

  tileset.save("pre-freeciv-web-tileset-" + tileset_name + "-" + str(tileset_inc) + ".png");
  tileset_inc += 1;

  tileset = Image.new('RGBA', (tileset_width, tileset_height), (0, 0, 0, 0));

  curr_x = 0;
  curr_y = 16;
  max_row_height = 0;
  max_width = 0;
  max_height = 0;

for tile_file in sorted(files.keys()):
  print("*** Extracting tileset: " + tile_file + "\n");
  tileset_inc = 0;
  coords[tile_file] = {};
  dither_map = {};

  for file in files[tile_file]:
    config = config_read(file);
    
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
          increment_tileset_image(tile_file);
        tileset.paste(im, (curr_x, curr_y));
        coords[tile_file][rsprite[0]] = (curr_x, curr_y, w, h, tileset_inc);
        curr_x += w;
        if (h > max_row_height): max_row_height = h;
        if (w + curr_x >= tileset_width): 
          curr_x = 0;
          curr_y += max_row_height;
          max_row_height = 0;
  
  
    if "file" in config.sections():
      gfx = config.get("file", "gfx") + ".png";
      print(gfx);
      im = Image.open(gfxdir + gfx.replace("\"", ""));
 
      for current_section in ["grid_main", "grid_roads", "grid_rails", "grid_coasts", "grid_extra"]: 
        if current_section in config.sections():
          dx = int(config.get(current_section, "dx"));
          dy = int(config.get(current_section, "dy"));
          x_top_left = int(config.get(current_section, "x_top_left"));
          y_top_left = int(config.get(current_section, "y_top_left"));
          pixel_border = 0;
          try:
            pixel_border = int(config.get(current_section, "pixel_border"));
          except:
            pass;
  
          print("pixel_border " + str(pixel_border));
          tag2 = None;
  
          tiles_buf = config.get(current_section, "tiles");
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

            if tag == "t.dither_tile":
              dither_mask = result_tile.copy();

            if tag == "mask.tile":
              mask_image = result_tile.copy();

  
            if (tag.find("cellgroup") != -1):
              #handle a cell group (1 tile = 4 cells)
              (WZ, HZ) = result_tile.size;
              xf = [int(WZ / 4), int(WZ / 4), 0, int(WZ / 2)];
              yf = [int(HZ / 2), 0, int(HZ / 4), int(HZ / 4)];
              xo = [0, 0, int(-WZ / 2), int(WZ / 2)];
              yo = [int(HZ / 2), int(-HZ / 2), 0, 0];
  
              for dir in range(4):
                result_cell = result_tile.copy().crop((xf[dir], yf[dir], int(xf[dir] + (WZ / 2)), int(yf[dir] + (HZ / 2))));
                mask = Image.new('RGBA', (int(WZ/2), int(HZ/2)), (255));
                mask.paste(mask_image, (int(xo[dir] - xf[dir]), int(yo[dir] - yf[dir])));
             
                (w, h) = result_cell.size;
                if (w > max_width): max_width = w;
                if (h > max_height): max_height = h;
                sum_area += (h*w);
  
                if (curr_y + h >= tileset_height):
                  increment_tileset_image(tile_file);
  
                tileset.paste(result_cell, (int(curr_x), int(curr_y)), mask);
                store_img = Image.new('RGBA', (w, h), (0, 0, 0, 0));
                store_img.paste(result_cell, (0, 0), mask);
                coords[tile_file][tag + "." + str(dir)] = (curr_x, curr_y, w, h, tileset_inc);
                curr_x += w;
                if (h > max_row_height): max_row_height = h;
                if (w + curr_x >= tileset_width): 
                  curr_x = 0;
                  curr_y += max_row_height;
                  max_row_height = 0;
  
            elif tag in dither_types:
              # handle a dithered tile.
              dither_map[tag] = result_tile.copy();
  
            elif tag.find("explode.nuke_") > -1 \
                 or tag.find("grid.borders") > -1 \
                 or tag.find("grid.selected") > -1 \
                 or tag.find("grid.main") > -1 \
                 or tag.find("grid.city") > -1 \
                 or tag.find("grid.coastline") > -1:
              continue; #skip unused tiles.
            else:
              # handle a non-cellgroup tile.
              (w, h) = result_tile.size;
              if (w > max_width): max_width = w;
              if (h > max_height): max_height = h;
              sum_area += (h*w);
  
              if (curr_y + h >= tileset_height):
                increment_tileset_image(tile_file);
  
              tileset.paste(result_tile, (curr_x, curr_y));
  
              coords[tile_file][tag] = (curr_x, curr_y, w, h, tileset_inc);
              curr_x += w;
              if (h > max_row_height): max_row_height = h;
              if (w + curr_x >= tileset_width): 
                curr_x = 0;
                curr_y += max_row_height;
                max_row_height = 0;
  
              if (tag2 != None and len(tag2) > 0): 
                coords[tile_file][tag2] = (curr_x, curr_y, w, h, tileset_inc);

  if not (tile_file == "amplio2" or tile_file == "isotrident"):  
    increment_tileset_image(tile_file);
  else: 
    for src_key in dither_map.keys():
      for alt_key in dither_map.keys():
          for dir in range(4):
            tag = str(dir) + src_key[5:].replace("1", "") + "_" + alt_key[5:].replace("1", "");
            src_img = dither_map[src_key].copy();
            alt_img = dither_map[alt_key].copy();
            src_img.paste(alt_img, None, dither_mask);

            (WZ, HZ) = src_img.size;
            xf = [int(WZ / 2), 0, int(WZ / 2), 0];
            yf = [0, int(HZ / 2), int(HZ / 2), 0];

            result_cell = src_img.crop((xf[dir], yf[dir], int(xf[dir] + (WZ / 2)), int(yf[dir] + (HZ / 2))));

            (w, h) = (int(WZ / 2) , int(HZ / 2));
            if (w > max_width): max_width = w;
            if (h > max_height): max_height = h;
            sum_area += (h*w);
            if (curr_y + h >= tileset_height):
              increment_tileset_image(tile_file);
            tileset.paste(result_cell, (curr_x, curr_y));

            coords[tile_file][tag] = (curr_x, curr_y, w, h, tileset_inc);

            curr_x += w;
            if (h > max_row_height): max_row_height = h;
            if (w + curr_x >= tileset_width): 
              curr_x = 0;
              curr_y += max_row_height;
              max_row_height = 0;

    increment_tileset_image(tile_file);

print("MAX: " + str(max_width) + "  " + str(max_height) + "  " + str(sum_area));

for tile_file in files.keys():
  f = open('tileset_spec_' + tile_file + '.js', 'w')
  f.write("var tileset = " + json.dumps(coords[tile_file], separators=(',',':')) + ";");


print("Freeciv-img-extract creating tilesets complete.");
