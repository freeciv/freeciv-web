#!/usr/bin/env python3
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

from os import environ, path
environ['PYTHON_EGG_CACHE'] = '/tmp'

import argparse
import configparser
from PIL import Image
from PIL import ImageDraw
import json

parser = argparse.ArgumentParser(
  description='Generate .js tileset specifications based on freeciv data')
parser.add_argument('-f', '--freeciv', required=True, help='path to (original) freeciv project')
parser.add_argument('-o', '--outdir', required=True, help='path to webapp output directory')
parser.add_argument('-v', '--verbose', action='store_true', help='show progress details during run')
args = parser.parse_args()

out_dir = args.outdir
freeciv_dir = args.freeciv
verbose = args.verbose

freeciv_data_dir = path.join(freeciv_dir, "data")

gfxdir = freeciv_data_dir

misc_files = [
  "wonders-large.spec",
  "colors.tilespec",
  "buildings-large.spec",
  "overlays.spec",
  "shields.spec",
  "small.spec",
  "governments.spec",
  "specialists.spec",
  "space.spec",
  "editor.spec",
  "techs.spec",
  "flags.spec",
  "treaty.spec",
  "citybar.spec"
]
spec_files = {
  "amplio2" : [
    "terrain1.spec",
    "volcano.spec",
    "activities.spec",
    "cities.spec",
    "bases.spec",
    "explosions.spec",
    "fog.spec",
    "grid.spec",
    "nuke.spec",
    "mountains.spec",
    "hills.spec",
    "ocean.spec",
    "select.spec",
    "terrain2.spec",
    "tiles.spec",
    "units.spec",
    "extra_units.spec",
    "upkeep.spec",
    "maglev.spec",
    "veterancy.spec",
    "water.spec"
  ],
  "trident" : [
    "auto_ll.spec",
    "fog.spec",
    "roads.spec",
    "tiles.spec",
    "cities.spec",
    "explosions.spec",
    "grid.spec",
    "select.spec",
    "units.spec"
  ],
  "isotrident" : [
    "terrain1.spec",
    "cities.spec",
    "fog.spec",
    "grid.spec",
    "morecities.spec",
    "nuke.spec",
    "select.spec",
    "terrain2.spec",
    "tiles.spec",
    "unitextras.spec"
  ]
}
def expand_spec_files(name, file_names):
  files = [path.join(freeciv_data_dir, name + ".tilespec")]
  files.extend([path.join(freeciv_data_dir, name, f) for f in file_names])
  files.extend([path.join(freeciv_data_dir, "misc", f) for f in misc_files])
  return files

files = {k: expand_spec_files(k, v) for k,v in spec_files.items()}

tileset = None;
curr_x = None;
curr_y = None;
max_row_height = None;
max_width = None;
max_height = None;
tileset_inc = None;
dither_map = None;
dither_mask = None;
mask_image = None;

coords = {};
max_width = 0;
max_height = 0;
sum_area = 0;
max_row_height = 0;
curr_x = 0;
curr_y = 14;
# Set size of tileset image manually depending on number of tiles.
# Note! Safari on iPhone doesn't support more than 3000000 pixels in a single image.
tileset_height = 1030;
tileset_width = 1800;

dither_types = ["t.l0.desert1", "t.l0.plains1", "t.l0.grassland1", "t.l0.forest1", "t.l0.jungle1", "t.l0.hills1", "t.l0.mountains1", "t.l0.tundra1", "t.l0.swamp1"];
print("Freeciv-img-extract running with PIL " + Image.__version__);
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
    if verbose: print("  found sections: {}".format(config.sections()))
    return config;

def increment_tileset_image(tileset_name):
  # Save current tileset file.
  global tileset;
  global curr_x;
  global curr_y;
  global max_row_height;
  global max_width;
  global max_height;
  global tileset_inc;

  draw = ImageDraw.Draw(tileset)
  draw.text((130, 0), "Freeciv-web - https://github.com/freeciv/freeciv-web  GPL Licensed  - Copyright 2007-2015  Andreas Rosdal", fill="rgb(0,0,0)")

  tileset_file = "freeciv-web-tileset-" + tileset_name + "-" + str(tileset_inc) + ".png"
  tileset.save(path.join(out_dir, tileset_file))
  tileset_inc += 1;

  tileset = Image.new('RGBA', (tileset_width, tileset_height), (0, 0, 0, 0));

  curr_x = 0;
  curr_y = 16;
  max_row_height = 0;
  max_width = 0;
  max_height = 0;

for tileset_id in sorted(files.keys()):
  print("*** Extracting tileset: " + tileset_id);
  tileset_inc = 0;
  coords[tileset_id] = {};
  dither_map = {};

  for file in files[tileset_id]:
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

        gfx_file = path.join(gfxdir, tag + ".png")
        im = Image.open(gfx_file)
        (w, h) = im.size;
        if (w > max_width): max_width = w;
        if (h > max_height): max_height = h;
        sum_area += (h*w);

        if (curr_y + h >= tileset_height):
          increment_tileset_image(tileset_id);
        tileset.paste(im, (curr_x, curr_y));
        coords[tileset_id][rsprite[0]] = (curr_x, curr_y, w, h, tileset_inc);
        curr_x += w;
        if (h > max_row_height): max_row_height = h;
        if (w + curr_x >= tileset_width):
          curr_x = 0;
          curr_y += max_row_height;
          max_row_height = 0;


    if "file" in config.sections():
      gfx = config.get("file", "gfx").replace("\"", "")
      gfx_file = path.join(gfxdir, gfx + ".png")
      if verbose: print("  processing: " + gfx_file)
      im = Image.open(gfx_file)

      for current_section in ["grid_main", "grid_roads", "grid_rails", "grid_maglev", "grid_coasts", "grid_extra"]:
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

          if verbose: print("  {} pixel_border {}".format(current_section, pixel_border))
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
              # Handle a cell group (1 tile = 4 cells)
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
                  increment_tileset_image(tileset_id);

                tileset.paste(result_cell, (int(curr_x), int(curr_y)), mask);
                store_img = Image.new('RGBA', (w, h), (0, 0, 0, 0));
                store_img.paste(result_cell, (0, 0), mask);
                coords[tileset_id][tag + "." + str(dir)] = (curr_x, curr_y, w, h, tileset_inc);
                curr_x += w;
                if (h > max_row_height): max_row_height = h;
                if (w + curr_x >= tileset_width):
                  curr_x = 0;
                  curr_y += max_row_height;
                  max_row_height = 0;

            elif tag in dither_types:
              # Handle a dithered tile.
              dither_map[tag] = result_tile.copy();

            elif tag.find("explode.nuke_") > -1 \
                 or tag.find("grid.borders") > -1 \
                 or tag.find("grid.selected") > -1 \
                 or tag.find("grid.main") > -1 \
                 or tag.find("grid.city") > -1 \
                 or tag.find("grid.coastline") > -1:
              continue; # Skip unused tiles.
            else:
              # Handle a non-cellgroup tile.
              (w, h) = result_tile.size;
              if (w > max_width): max_width = w;
              if (h > max_height): max_height = h;
              sum_area += (h*w);

              if (curr_y + h >= tileset_height):
                increment_tileset_image(tileset_id);

              tileset.paste(result_tile, (curr_x, curr_y));

              coords[tileset_id][tag] = (curr_x, curr_y, w, h, tileset_inc);
              curr_x += w;
              if (h > max_row_height): max_row_height = h;
              if (w + curr_x >= tileset_width):
                curr_x = 0;
                curr_y += max_row_height;
                max_row_height = 0;

              if (tag2 != None and len(tag2) > 0):
                coords[tileset_id][tag2] = (curr_x, curr_y, w, h, tileset_inc);

  if not (tileset_id == "amplio2" or tileset_id == "isotrident"):
    increment_tileset_image(tileset_id);
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
              increment_tileset_image(tileset_id);
            tileset.paste(result_cell, (curr_x, curr_y));

            coords[tileset_id][tag] = (curr_x, curr_y, w, h, tileset_inc);

            curr_x += w;
            if (h > max_row_height): max_row_height = h;
            if (w + curr_x >= tileset_width):
              curr_x = 0;
              curr_y += max_row_height;
              max_row_height = 0;

    increment_tileset_image(tileset_id);

if verbose: print("MAX: " + str(max_width) + "  " + str(max_height) + "  " + str(sum_area));

for tileset_id in files.keys():
  tileset_file = 'tileset_spec_' + tileset_id + '.js'
  with open(path.join(out_dir, tileset_file), 'w') as f:
    f.write("var tileset = " + json.dumps(coords[tileset_id],
                                          separators=(',', ':')) + ";")

print("Freeciv-img-extract creating tilesets complete.");
