<!-- GLSL Fragment Shader for Freeciv-web -->
<script id="fragment_shh" type="x-shader/x-fragment">
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
    uniform sampler2D heightmap;

    uniform int mapWidth;
    uniform int mapHeight;

    varying vec2 vUv;

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


    void main(void)
    {

        vec4 terrain_type = texture2D(maptiles, vec2(vUv.x, vUv.y));
        vec3 c;

        /* Set pixel color based on tile type. */
        if (terrain_type.r + 0.02 > terrain_lake && terrain_type.r - 0.02 < terrain_lake) {
          vec4 Ca = texture2D(lake, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Ca.rgb;
        } else if (terrain_type.r == terrain_coast) {
          vec4 Cb = texture2D(coast, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Cb.rgb;
        } else if (terrain_type.r == terrain_floor) {
          vec4 Cb = texture2D(floor, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Cb.rgb;
        } else if (terrain_type.r == terrain_arctic) {
          vec4 Cb = texture2D(arctic, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Cb.rgb;
        } else if (terrain_type.r + 0.02 > terrain_desert && terrain_type.r - 0.02 < terrain_desert) {
          vec4 Cb = texture2D(desert, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Cb.rgb;
        } else if (terrain_type.r + 0.02 > terrain_forest && terrain_type.r - 0.02 < terrain_forest) {
          vec4 Cb = texture2D(forest, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Cb.rgb;
        } else if (terrain_type.r + 0.02 > terrain_grassland && terrain_type.r - 0.02 < terrain_grassland) {
          vec4 Cb = texture2D(grassland, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Cb.rgb;
        } else if (terrain_type.r == terrain_hills) {
          vec4 Cb = texture2D(hills, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Cb.rgb;
        } else if (terrain_type.r == terrain_jungle) {
          vec4 Cb = texture2D(jungle, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Cb.rgb;
        } else if (terrain_type.r == terrain_mountains) {
          vec4 Cb = texture2D(mountains, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Cb.rgb;
        } else if (terrain_type.r + 0.02 > terrain_plains && terrain_type.r - 0.02 < terrain_plains) {
          vec4 Cb = texture2D(plains, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Cb.rgb;
        } else if (terrain_type.r == terrain_swamp) {
          vec4 Cb = texture2D(swamp, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Cb.rgb;
        } else if (terrain_type.r == terrain_tundra) {
          vec4 Cb = texture2D(tundra, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Cb.rgb;
        } else {
          vec4 Cb = texture2D(grassland, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Cb.rgb;
        }

        /* Render beach based on heightmap. */
        /*
        FIXME: this does not work yet.
        vec4 height_current = texture2D(heightmap, vec2(vUv.x, vUv.y));
        vec4 height_north = texture2D(heightmap, vec2(vUv.x, vUv.y + 10.0));
        vec4 height_south = texture2D(heightmap, vec2(vUv.x, vUv.y - 10.0));
        vec4 height_west = texture2D(heightmap, vec2(vUv.x + 10.0, vUv.y));
        vec4 height_east = texture2D(heightmap, vec2(vUv.x - 10.0, vUv.y));

        if (height_current.r == heightmap_land && height_south.r == heightmap_ocean) {
          vec4 Cb = texture2D(beach, vec2(vUv.x * 50.0, vUv.y * 50.0));
          c = Cb.rgb;
        }*/

        vec3 light = vec3(0.8, 0.6, 0.7);
        light = normalize(light);

        float dProd = max(0.0, dot(vNormal, light));


        gl_FragColor= vec4(c.r * dProd * 1.1, c.g * dProd * 1.1, c.b * dProd * 1.1, 1.0);

    }

</script>

