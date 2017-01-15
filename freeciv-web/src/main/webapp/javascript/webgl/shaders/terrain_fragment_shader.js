<!-- GLSL Fragment Shader for Freeciv-web -->
<script id="terrain_fragment_shh" type="x-shader/x-fragment">
/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2016  The Freeciv-web project

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

uniform sampler2D lake;
uniform sampler2D coast;
uniform sampler2D floor;
uniform sampler2D arctic;
uniform sampler2D desert;
uniform sampler2D forest;
uniform sampler2D grassland;
uniform sampler2D hills;
uniform sampler2D jungle;
uniform sampler2D mountains;
uniform sampler2D plains;
uniform sampler2D swamp;
uniform sampler2D tundra;
uniform sampler2D beach;

uniform sampler2D maptiles;

uniform int mapWidth;
uniform int mapHeight;

varying vec2 vUv;
varying vec3 vPosition;
varying vec3 vPosition_camera;

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

float heightmap_land = 10.0/255.0;
float heightmap_ocean = 0.0;


float beach_high = 55.0;
float beach_blend_high = 53.0;
float beach_blend_low = 50.0;
float beach_low = 49.0;
float blend_amount = 0.0;

float mountains_low_begin = 98.0;
float mountains_low_end = 100.0;
float mountains_high = 115.0;


void main(void)
{

    vec4 terrain_type = texture2D(maptiles, vec2(vUv.x, vUv.y));

    vec3 c;
    vec4 chosen_terrain_color;

    /* Set pixel color based on tile type. */
    if (terrain_type.r == terrain_grassland) {
      if (vPosition.y > beach_blend_high ) {
        chosen_terrain_color = texture2D(grassland, vec2(vUv.x * 50.0, vUv.y * 50.0));
      } else {
        chosen_terrain_color = texture2D(coast, vec2(vUv.x * 50.0, vUv.y * 50.0));
      }
    } else if (terrain_type.r == terrain_plains) {
      if (vPosition.y > beach_blend_high ) {
        chosen_terrain_color = texture2D(plains, vec2(vUv.x * 50.0, vUv.y * 50.0));
      } else {
        chosen_terrain_color = texture2D(coast, vec2(vUv.x * 50.0, vUv.y * 50.0));
      }
    } else if (terrain_type.r == terrain_lake) {
      if (vPosition.y < beach_blend_high ) {
        chosen_terrain_color = texture2D(lake, vec2(vUv.x * 50.0, vUv.y * 50.0));
      } else {
        chosen_terrain_color = texture2D(grassland, vec2(vUv.x * 50.0, vUv.y * 50.0));
      }
    } else if (terrain_type.r == terrain_coast) {
      if (vPosition.y < beach_blend_high ) {
        chosen_terrain_color = texture2D(coast, vec2(vUv.x * 50.0, vUv.y * 50.0));
      } else {
        chosen_terrain_color = texture2D(grassland, vec2(vUv.x * 50.0, vUv.y * 50.0));
      }
    } else if (terrain_type.r == terrain_floor) {
      if (vPosition.y < beach_blend_high ) {
        chosen_terrain_color = texture2D(floor, vec2(vUv.x * 50.0, vUv.y * 50.0));
      } else {
        chosen_terrain_color = texture2D(grassland, vec2(vUv.x * 50.0, vUv.y * 50.0));
      }
    } else if (terrain_type.r == terrain_arctic) {
      chosen_terrain_color = texture2D(arctic, vec2(vUv.x * 50.0, vUv.y * 50.0));
    } else if (terrain_type.r == terrain_desert) {
      if (vPosition.y > beach_blend_high ) {
        chosen_terrain_color = texture2D(desert, vec2(vUv.x * 50.0, vUv.y * 50.0));
      } else {
        chosen_terrain_color = texture2D(coast, vec2(vUv.x * 50.0, vUv.y * 50.0));
      }
    } else if (terrain_type.r == terrain_forest) {
      if (vPosition.y > beach_blend_high ) {
        chosen_terrain_color = texture2D(forest, vec2(vUv.x * 50.0, vUv.y * 50.0));
      } else {
        chosen_terrain_color = texture2D(coast, vec2(vUv.x * 50.0, vUv.y * 50.0));
      }
    } else if (terrain_type.r == terrain_hills) {
      if (vPosition.y > beach_blend_high ) {
        chosen_terrain_color = texture2D(hills, vec2(vUv.x * 50.0, vUv.y * 50.0));
      } else {
        chosen_terrain_color = texture2D(coast, vec2(vUv.x * 50.0, vUv.y * 50.0));
      }
    } else if (terrain_type.r == terrain_jungle) {
      if (vPosition.y > beach_blend_high ) {
        chosen_terrain_color = texture2D(jungle, vec2(vUv.x * 50.0, vUv.y * 50.0));
      } else {
        chosen_terrain_color = texture2D(coast, vec2(vUv.x * 50.0, vUv.y * 50.0));
      }
    } else if (terrain_type.r == terrain_mountains) {
      if (vPosition.y > beach_blend_high ) {
        chosen_terrain_color = texture2D(mountains, vec2(vUv.x * 50.0, vUv.y * 50.0));
      } else {
        chosen_terrain_color = texture2D(coast, vec2(vUv.x * 50.0, vUv.y * 50.0));
      }
    } else if (terrain_type.r == terrain_swamp) {
      if (vPosition.y > beach_blend_high ) {
        chosen_terrain_color = texture2D(swamp, vec2(vUv.x * 50.0, vUv.y * 50.0));
      } else {
        chosen_terrain_color = texture2D(coast, vec2(vUv.x * 50.0, vUv.y * 50.0));
      }
    } else if (terrain_type.r == terrain_tundra) {
      if (vPosition.y > beach_blend_high ) {
        chosen_terrain_color = texture2D(tundra, vec2(vUv.x * 50.0, vUv.y * 50.0));
      } else {
        chosen_terrain_color = texture2D(coast, vec2(vUv.x * 50.0, vUv.y * 50.0));
      }
    } else {
      if (vPosition.y > beach_blend_high ) {
        chosen_terrain_color = texture2D(grassland, vec2(vUv.x * 50.0, vUv.y * 50.0));
      } else {
        chosen_terrain_color = texture2D(coast, vec2(vUv.x * 50.0, vUv.y * 50.0));
      }
    }
    c = chosen_terrain_color.rgb;

  /* render the beach. */
  if (vPosition.y < beach_high && vPosition.y > beach_low) {
    if (vPosition.y > beach_blend_high) {
      blend_amount = (2.0 - (beach_high - vPosition.y)) / 2.0;
      vec4 Cbeach = texture2D(beach, vec2(vUv.x * 50.0, vUv.y * 50.0));
      c = chosen_terrain_color.rgb * blend_amount + (Cbeach.rgb * (1.0 - blend_amount));

    } else if (vPosition.y < beach_blend_low) {
      blend_amount = ((beach_blend_low - vPosition.y)) / 1.0;
      vec4 Cbeach = texture2D(beach, vec2(vUv.x * 50.0, vUv.y * 50.0));
      c = chosen_terrain_color.rgb * blend_amount + (Cbeach.rgb * (1.0 - blend_amount));

    } else {
      vec4 Cbeach = texture2D(beach, vec2(vUv.x * 50.0, vUv.y * 50.0));
      c = Cbeach.rgb;
    }

  }

  if (vPosition.y > mountains_high) {
      /* snow in mountains texture over a certain height threshold. */
      blend_amount = ((3.0 - (mountains_high - vPosition.y)) / 3.0) - 1.0;
      vec4 Ca = texture2D(arctic, vec2(vUv.x * 50.0, vUv.y * 50.0));
      vec4 Cb = texture2D(mountains, vec2(vUv.x * 50.0, vUv.y * 50.0));
      c = Ca.rgb * blend_amount + Cb.rgb * (1.0 - blend_amount);
  } else if (vPosition.y > mountains_low_begin) {
      if (vPosition.y < mountains_low_end) {
        vec4 Cmountain = texture2D(mountains, vec2(vUv.x * 50.0, vUv.y * 50.0));
        c = chosen_terrain_color.rgb * (1.0 - smoothstep(mountains_low_begin, mountains_low_end, vPosition.y))
            + Cmountain.rgb * smoothstep(mountains_low_begin, mountains_low_end, vPosition.y);
      } else {
        /* mountain texture over a certain height threshold. */
        vec4 Cb = texture2D(mountains, vec2(vUv.x * 50.0, vUv.y * 50.0));
        c = Cb.rgb;
      }
  }

  /* specular component, ambient occlusion and fade out underwater terrain */
  float x = clamp((vPosition.y - 30.) / 15., 0., 1.);
  vec4 Cb = texture2D(beach, vec2(vUv.x * 50.0, vUv.y * 50.0));
  c = Cb.rgb * (1. - x) + c * x;

  vec3 light = vec3(0.8, 0.6, 0.7);
  light = normalize(light);

  float dProd = dot(vNormal, light);
  float shade_factor = 0.4 + 0.6 * max(0., dProd);

  vec3 ambiant = vec3(0.27, 0.55, 1.);
  float ambiant_factor = 0.075 * (vPosition_camera.z - 550.) / 400.;
  gl_FragColor.rgb = (1. - ambiant_factor) * c * shade_factor + ambiant * ambiant_factor;

}

</script>

