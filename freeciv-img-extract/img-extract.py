#!/usr/bin/python
# -*- coding: iso-8859-1 -*-

import os, sys

# Copyright 2009, Andreas Røsdal

import ConfigParser
import Image
import ImageFont, ImageDraw, ImageOps
import json

gfxdir = "../freeciv/data/";

files = ["../freeciv/data/amplio.tilespec",
  "../freeciv/data/amplio/ancientcities.spec",
  "../freeciv/data/amplio/buildings.spec",
  "../freeciv/data/amplio/explosions.spec",
  "../freeciv/data/amplio/fog.spec",
  "../freeciv/data/amplio/grid.spec",
  "../freeciv/data/amplio/icons.spec",
  "../freeciv/data/amplio/medievalcities.spec",
  "../freeciv/data/amplio/moderncities.spec",
  "../freeciv/data/amplio/nuke.spec",
  "../freeciv/data/amplio/ocean.spec",
  "../freeciv/data/amplio/select.spec",
  "../freeciv/data/amplio/terrain1.spec",
  "../freeciv/data/amplio/terrain2.spec",
  "../freeciv/data/amplio/tiles.spec",
  "../freeciv/data/amplio/units.spec",
  "../freeciv/data/amplio/water.spec",
  "../freeciv/data/amplio/wonders.spec",
  "../freeciv/data/misc/colors.tilespec",
  "../freeciv/data/misc/buildings.spec",
  "../freeciv/data/misc/icons.spec",
#  "../freeciv/data/misc/chiefs.spec",
  "../freeciv/data/misc/overlays.spec",
  "../freeciv/data/misc/citybar.spec",
  "../freeciv/data/misc/shields.spec",
  "../freeciv/data/misc/small.spec",
  "../freeciv/data/misc/cursors.spec",
  "../freeciv/data/misc/space.spec",
  "../freeciv/data/misc/editor.spec",
  "../freeciv/data/misc/techs.spec",
  "../freeciv/data/misc/flags.spec",
  "../freeciv/data/misc/treaty.spec"];

global tileset;
global current_tileset_no;
global curr_x;
global curr_y;
global max_row_height;
global max_width;
global max_height;

coords = {};
max_width = 0;
max_height = 0;
sum_area = 0;
max_row_height = 0;
curr_x = 0;
curr_y = 14;
tileset_height = 1800;  #FIXME: This must be adjusted according to the number of tiles. # Stupid Flash 9 limitation of 2880.
tileset_width = 1280;

current_tileset_no = 1;
tileset = Image.new('RGBA', (tileset_width, tileset_height), (0, 0, 0, 0));
mask_image = Image.open("mask.png");

def increment_tileset_image():
  # save current tileset file.
  global tileset;
  global current_tileset_no;
  global curr_x;
  global curr_y;
  global max_row_height;
  global max_width;
  global max_height;

  draw = ImageDraw.Draw(tileset)
  draw.text((130, 0), "Freeciv Web Client   - Amplio tileset number " + str(current_tileset_no) + " -   GPL Licensed  -   Web Client Copyright 2007-2010  Andreas Rosdal", fill="rgb(0,0,0)")

  tileset.save("freeciv-web-tileset-" + str(current_tileset_no) + ".png");

  current_tileset_no += 1;
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
      im.save("tiles/" + rsprite[0] + ".png");
      coords[rsprite[0]] = (current_tileset_no, curr_x, curr_y, w, h);
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
        if (tmptile[0] == "row"): continue;
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

        if (tag == "t.l0.grassland1"):
          result_tile = Image.open("grassland1.png");
        if (tag == "t.l0.plains1"):
          result_tile = Image.open("plains1.png");

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
            store_img.save("tiles/" + tag + "." + str(dir) + ".png");
            coords[tag + "." + str(dir)] = (current_tileset_no, curr_x, curr_y, w, h);
            curr_x += w;
            if (h > max_row_height): max_row_height = h;
            if (w + curr_x >= tileset_width): 
              curr_x = 0;
              curr_y += max_row_height;
              max_row_height = 0;


        else:
          # handle a non-cellgroup tile.
          (w, h) = result_tile.size;
          if (w > max_width): max_width = w;
          if (h > max_height): max_height = h;
          sum_area += (h*w);

          if (curr_y + h >= tileset_height):
            increment_tileset_image();

          tileset.paste(result_tile, (curr_x, curr_y));
          result_tile.save("tiles/" + tag + ".png");

          coords[tag] = (current_tileset_no, curr_x, curr_y, w, h);
          curr_x += w;
          if (h > max_row_height): max_row_height = h;
          if (w + curr_x >= tileset_width): 
            curr_x = 0;
            curr_y += max_row_height;
            max_row_height = 0;

          #if (tag2 != None): result_tile.save(tag2 + ".png"); 
          if (tag2 != None): 
            coords[tag2] = (current_tileset_no, curr_x, curr_y, w, h);
            result_tile.save("tiles/" + tag2 + ".png");
 
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
        result_tile.save("tiles/" + tag + ".png");

        coords[tag] = (current_tileset_no, curr_x, curr_y, w, h);
        curr_x += w;
        if (h > max_row_height): max_row_height = h;
        if (w + curr_x >= tileset_width): 
          curr_x = 0;
          curr_y += max_row_height;
          max_row_height = 0;

        #if (tag2 != None): result_tile.save(tag2 + ".png"); 
        if (tag2 != None): 
          coords[tag2] = (current_tileset_no, curr_x, curr_y, w, h);
          result_tile.save("tiles/" + tag + ".png");


increment_tileset_image();

print("MAX: " + str(max_width) + "  " + str(max_height) + "  " + str(sum_area));

f = open('freeciv-web-tileset.js', 'w')

f.write("var tileset = " + json.dumps(coords, separators=(',',':')));


f_sm = open('freeciv-web-tileset-small-preload.html', 'w')
for sp_key in coords:
  f_sm.write("<img src='/tiles/" + sp_key + ".png' width='0' height='0'>");

print("done.");
