/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2017  The Freeciv-web project

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************/

#ifdef GL_ES
precision highp float;
#endif

varying vec3 vNormal;
varying vec3 vColor;

uniform sampler2D terrains;
uniform sampler2D maptiles;
uniform sampler2D borders;
uniform sampler2D roadsmap;
uniform sampler2D roadsprites;
uniform sampler2D railroadsprites;

uniform float map_x_size;
uniform float map_y_size;

varying vec2 vUv;
varying vec3 vPosition;
varying vec3 vPosition_camera;


float sprite_pos0_x = 0.0;
float sprite_pos0_y = 0.75;
float sprite_pos1_x = 0.25;
float sprite_pos1_y = 0.75;
float sprite_pos2_x = 0.5;
float sprite_pos2_y = 0.75;
float sprite_pos3_x = 0.75;
float sprite_pos3_y = 0.75;
float sprite_pos4_x = 0.0;
float sprite_pos4_y = 0.5;
float sprite_pos5_x = 0.25;
float sprite_pos5_y = 0.5;
float sprite_pos6_x = 0.5;
float sprite_pos6_y = 0.5;
float sprite_pos7_x = 0.75;
float sprite_pos7_y = 0.5;
float sprite_pos8_x = 0.0;
float sprite_pos8_y = 0.25;
float sprite_pos9_x = 0.25;
float sprite_pos9_y = 0.25;
float sprite_pos10_x = 0.5;
float sprite_pos10_y = 0.25;
float sprite_pos11_x = 0.75;
float sprite_pos11_y = 0.25;
float sprite_pos12_x = 0.0;
float sprite_pos12_y = 0.0;
float sprite_pos13_x = 0.25;
float sprite_pos13_y = 0.0;
float sprite_pos14_x = 0.5;
float sprite_pos14_y = 0.0;
float sprite_pos15_x = 0.75;
float sprite_pos15_y = 0.0;

float terrain_inaccessible = 0.0;
float terrain_lake = 10.0/255.0;
float terrain_coast = 20.0/255.0;
float terrain_floor = 30.0/255.0;
float terrain_arctic = 40.0/255.0;
float terrain_desert = 50.0/255.0;
float terrain_forest = 60.0/255.0;
float terrain_grassland = 70.0/255.0;
float terrain_hills = 80.0/255.0;
float terrain_jungle = 90.0/255.0;
float terrain_mountains = 100.0/255.0;
float terrain_plains = 110.0/255.0;
float terrain_swamp = 120.0/255.0;
float terrain_tundra = 130.0/255.0;

float is_river_modifier = 10.0/255.0;

// roads
float roadtype_1 = 1.0/255.0;
float roadtype_2 = 2.0/255.0;
float roadtype_3 = 3.0/255.0;
float roadtype_4 = 4.0/255.0;
float roadtype_5 = 5.0/255.0;
float roadtype_6 = 6.0/255.0;
float roadtype_7 = 7.0/255.0;
float roadtype_8 = 8.0/255.0;
float roadtype_9 = 9.0/255.0;
float roadtype_all = 42.0/255.0;

// railroads
float roadtype_10 = 10.0/255.0;
float roadtype_11 = 11.0/255.0;
float roadtype_12 = 12.0/255.0;
float roadtype_13 = 13.0/255.0;
float roadtype_14 = 14.0/255.0;
float roadtype_15 = 15.0/255.0;
float roadtype_16 = 16.0/255.0;
float roadtype_17 = 17.0/255.0;
float roadtype_18 = 18.0/255.0;
float roadtype_19 = 19.0/255.0;
float railtype_all = 43.0/255.0;

float beach_high = 53.5;
float beach_blend_high = 52.0;
float beach_blend_low = 50.0;
float beach_low = 44.0;
float blend_amount = 0.0;

float mountains_low_begin = 93.0;
float mountains_low_end = 95.0;
float mountains_high = 100.2;

vec3 ambiant = vec3(0.27, 0.55, 1.);
vec3 light = vec3(0.8, 0.6, 0.7);

vec2 texture_coord;


void main(void)
{

    if (vColor.r == 0.0) {
      gl_FragColor.rgb = vec3(0, 0, 0);
      return;
    }

    vec4 terrain_type = texture2D(maptiles, vec2(vUv.x, vUv.y));
    vec4 border_color = texture2D(borders, vec2(vUv.x, vUv.y));
    vec4 road_type = texture2D(roadsmap, vec2(vUv.x, vUv.y));

    vec3 c;
    vec4 chosen_terrain_color;

    if (terrain_type.g == is_river_modifier) {
      beach_high = 50.5;
      beach_blend_high = 50.25;
    }

    // Set pixel color based on tile type.
    if (terrain_type.r == terrain_grassland) {
      if (vPosition.y > beach_blend_high ) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos6_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos6_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      } else {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos2_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos2_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      }
    } else if (terrain_type.r == terrain_plains) {
      if (vPosition.y > beach_blend_high ) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos11_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos11_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      } else {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos2_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos2_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      }
    } else if (terrain_type.r == terrain_lake) {
      if (vPosition.y < beach_blend_high ) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos9_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos9_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      } else {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos6_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos6_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      }
    } else if (terrain_type.r == terrain_coast) {
      if (vPosition.y < beach_blend_high ) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos2_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos2_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
        if (fract((vPosition.x + 502.0) / 35.71) < 0.018 || fract((vPosition.z + 2.0) / 35.71) < 0.018) {
          chosen_terrain_color.rgb = chosen_terrain_color.rgb * 1.45;  // render tile grid.
        }

      } else {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos6_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos6_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      }
    } else if (terrain_type.r == terrain_floor) {
      if (vPosition.y < beach_blend_high ) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos4_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos4_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
        if (fract((vPosition.x + 502.0) / 35.71) < 0.018 || fract((vPosition.z + 2.0) / 35.71) < 0.018) {
          chosen_terrain_color.rgb = chosen_terrain_color.rgb * 1.7;  // render tile grid.
        }
      } else {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos6_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos6_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      }
    } else if (terrain_type.r == terrain_arctic) {
      texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos0_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos0_y);
      chosen_terrain_color = texture2D(terrains, texture_coord);
    } else if (terrain_type.r == terrain_desert) {
      if (vPosition.y > beach_blend_high ) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos3_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos3_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      } else {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos2_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos2_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      }
    } else if (terrain_type.r == terrain_forest) {
      if (vPosition.y > beach_blend_high ) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos5_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos5_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      } else {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos2_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos2_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      }
    } else if (terrain_type.r == terrain_hills) {
      if (vPosition.y > beach_blend_high ) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos7_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos7_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      } else {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos2_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos2_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      }
    } else if (terrain_type.r == terrain_jungle) {
      if (vPosition.y > beach_blend_high ) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos8_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos8_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      } else {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos2_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos2_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      }
    } else if (terrain_type.r == terrain_mountains) {
      if (vPosition.y > beach_blend_high ) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos10_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos10_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      } else {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos2_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos2_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      }
    } else if (terrain_type.r == terrain_swamp) {
      if (vPosition.y > beach_blend_high ) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos12_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos12_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      } else {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos2_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos2_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      }
    } else if (terrain_type.r == terrain_tundra) {
      if (vPosition.y > beach_blend_high ) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos13_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos13_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      } else {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos2_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos2_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      }
    } else {
      if (vPosition.y > beach_blend_high ) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos11_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos11_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      } else {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos2_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos2_y);
        chosen_terrain_color = texture2D(terrains, texture_coord);
      }
    }

  c = chosen_terrain_color.rgb;

  if (vPosition.y > mountains_high) {
      // snow in mountains texture over a certain height threshold.
      blend_amount = ((3.0 - (mountains_high - vPosition.y)) / 3.0) - 1.0;

      vec4 Ca = texture2D(terrains, vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos0_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos0_y));
      vec4 Cb = texture2D(terrains, vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos10_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos10_y));
      c = mix(Ca.rgb, Cb.rgb, (1.0 - blend_amount));
  } else if (vPosition.y > mountains_low_begin) {
      if (vPosition.y < mountains_low_end) {
        vec4 Cmountain = texture2D(terrains, vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos10_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos10_y));
        c = mix(chosen_terrain_color.rgb, Cmountain.rgb, smoothstep(mountains_low_begin, mountains_low_end, vPosition.y));
      } else {
        // mountain texture over a certain height threshold.
        vec4 Cb = texture2D(terrains, vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos10_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos10_y));
        c = Cb.rgb;
      }
  }


  if (fract((vPosition.x + 502.0) / 35.71) < 0.018 || fract((vPosition.z + 2.0) / 35.71) < 0.018) {
    c = c - 0.085;  // render tile grid.
  }


  // render the beach.
  if (vPosition.y < beach_high && vPosition.y > beach_low) {
    texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos1_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos1_y);
    if (vPosition.y > beach_blend_high) {
      blend_amount = ((beach_high - beach_blend_high) - (beach_high - vPosition.y)) / (beach_high - beach_blend_high);
      vec4 Cbeach = texture2D(terrains, texture_coord);
      c = mix(chosen_terrain_color.rgb, Cbeach.rgb, (1.0 - blend_amount));

    } else if (vPosition.y < beach_blend_low) {
      blend_amount = (beach_blend_low - vPosition.y) / 6.0;
      vec4 Cbeach = texture2D(terrains, texture_coord) * 2.0;
      c = mix(chosen_terrain_color.rgb, Cbeach.rgb, (1.0 - blend_amount));

    } else {
      vec4 Cbeach = texture2D(terrains, texture_coord);
      c = Cbeach.rgb;
    }
  }

  if (vColor.g > 0.4 && vColor.g < 0.6) {
    // render Irrigation.
    texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos15_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos15_y);
    vec4 t1 = texture2D(terrains, texture_coord);
    c = mix(c, vec3(t1), t1.a);
  }

  if (vColor.g > 0.9) {
    // render farmland.
    texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos14_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos14_y);
    vec4 t1 = texture2D(terrains, texture_coord);
    c = mix(c, vec3(t1), t1.a);
  }

  // Roads
  if (road_type.r == 0.0) {
      // no roads
  } else if (road_type.r == roadtype_1 && road_type.g == 0.0 &&  road_type.b == 0.0) {
      // a single road tile.
      texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos0_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos0_y);
      vec4 t1 = texture2D(roadsprites, texture_coord);
      c = mix(c, vec3(t1), t1.a);
  } else if (road_type.r == roadtype_all) {
      // a road tile with 4 connecting roads.
      texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos1_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos1_y);
      vec4 t1 = texture2D(roadsprites, texture_coord);
      c = mix(c, vec3(t1), t1.a);
      texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos3_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos3_y);
      t1 = texture2D(roadsprites, texture_coord);
      c = mix(c, vec3(t1), t1.a);
      texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos5_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos5_y);
      t1 = texture2D(roadsprites, texture_coord);
      c = mix(c, vec3(t1), t1.a);
      texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos7_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos7_y);
      t1 = texture2D(roadsprites, texture_coord);
      c = mix(c, vec3(t1), t1.a);
  } else if (road_type.r == railtype_all) {
      // a rail tile with 4 connecting rails.
      texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos1_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos1_y);
      vec4 t1 = texture2D(railroadsprites, texture_coord);
      c = mix(c, vec3(t1), t1.a);
      texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos3_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos3_y);
      t1 = texture2D(railroadsprites, texture_coord);
      c = mix(c, vec3(t1), t1.a);
      texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos5_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos5_y);
      t1 = texture2D(railroadsprites, texture_coord);
      c = mix(c, vec3(t1), t1.a);
      texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos7_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos7_y);
      t1 = texture2D(railroadsprites, texture_coord);
      c = mix(c, vec3(t1), t1.a);
  } else if (road_type.r > 0.0 && road_type.r < roadtype_10) {
      // Roads
      if (road_type.r == roadtype_2 || road_type.g == roadtype_2 || road_type.b == roadtype_2) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos1_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos1_y);
        vec4 t1 = texture2D(roadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_3 || road_type.g == roadtype_3 || road_type.b == roadtype_3) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos2_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos2_y);
        vec4 t1 = texture2D(roadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_4 || road_type.g == roadtype_4 || road_type.b == roadtype_4) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos3_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos3_y);
        vec4 t1 = texture2D(roadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_5 || road_type.g == roadtype_5 || road_type.b == roadtype_5) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos4_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos4_y);
        vec4 t1 = texture2D(roadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_6 || road_type.g == roadtype_6 || road_type.b == roadtype_6) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos5_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos5_y);
        vec4 t1 = texture2D(roadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_7 || road_type.g == roadtype_7 || road_type.b == roadtype_7) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos6_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos6_y);
        vec4 t1 = texture2D(roadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_8 || road_type.g == roadtype_8 || road_type.b == roadtype_8) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos7_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos7_y);
        vec4 t1 = texture2D(roadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_9 || road_type.g == roadtype_9 || road_type.b == roadtype_9) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos8_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos8_y);
        vec4 t1 = texture2D(roadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
  } else if (road_type.r >= roadtype_10 && road_type.r < roadtype_all) {
      // Railroads
      if (road_type.r == roadtype_10 && road_type.g == 0.0 &&  road_type.b == 0.0) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos0_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos0_y);
        vec4 t1 = texture2D(railroadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_12 || road_type.g == roadtype_12 || road_type.b == roadtype_12) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos1_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos1_y);
        vec4 t1 = texture2D(railroadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_13 || road_type.g == roadtype_13 || road_type.b == roadtype_13) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos2_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos2_y);
        vec4 t1 = texture2D(railroadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_14 || road_type.g == roadtype_14 || road_type.b == roadtype_14) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos3_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos3_y);
        vec4 t1 = texture2D(railroadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_15 || road_type.g == roadtype_15 || road_type.b == roadtype_15) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos4_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos4_y);
        vec4 t1 = texture2D(railroadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_16 || road_type.g == roadtype_16 || road_type.b == roadtype_16) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos5_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos5_y);
        vec4 t1 = texture2D(railroadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_17 || road_type.g == roadtype_17 || road_type.b == roadtype_17) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos6_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos6_y);
        vec4 t1 = texture2D(railroadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_18 || road_type.g == roadtype_18 || road_type.b == roadtype_18) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos7_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos7_y);
        vec4 t1 = texture2D(railroadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
      if (road_type.r == roadtype_19 || road_type.g == roadtype_19 || road_type.b == roadtype_19) {
        texture_coord = vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos8_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos8_y);
        vec4 t1 = texture2D(railroadsprites, texture_coord);
        c = mix(c, vec3(t1), t1.a);
      }
  }


  // Borders
  if (!(border_color.r > 0.546875 && border_color.r < 0.5625 && border_color.b == 0.0 && border_color.g == 0.0)) {
    c = mix(c, border_color.rbg, 0.53);
  }

  // specular component, ambient occlusion and fade out underwater terrain
  float x = 1.0 - clamp((vPosition.y - 30.) / 15., 0., 1.);
  vec4 Cb = texture2D(terrains, vec2(mod(map_x_size * (vUv.x / 4.0), 0.25) + sprite_pos1_x , mod((vUv.y * map_y_size / 4.0), 0.25) + sprite_pos1_y));
  c = mix(c, Cb.rgb, x);

  float shade_factor = 0.19 + 1.3 * max(0., dot(vNormal, normalize(light)));

  // Fog of war, and unknown tiles, are stored as a vertex color in vColor.r.
  c = c * vColor.r;

  gl_FragColor.rgb = mix(c * shade_factor, ambiant, (vPosition_camera.z - 550.) * 0.0001875);

}
