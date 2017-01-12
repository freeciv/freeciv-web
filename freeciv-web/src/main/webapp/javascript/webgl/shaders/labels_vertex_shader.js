<!-- GLSL Vertex Shader for Freeciv-web -->

<script id="labels_vertex_shh" type="x-shader/x-vertex">
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

uniform float u_scale_factor;

// Unit vectors
const vec4 ux = vec4(1., 0., 0., 0.);
const vec4 uy = vec4(0., 1., 0., 0.);
const vec4 uz = vec4(0., 0., 1., 0.);
const vec4 uw = vec4(0., 0., 0., 1.);

varying vec2 vUv;

void main()
{
  vUv = vec2(uv.x * u_scale_factor, uv.y);
  // Extract scalings from the projection matrix
  // 0.7 = sqrt(2)/2 is there because x is projected at 45° by the view matrix
  float scale_x = 0.7 * dot(projectionMatrix * ux, ux);
  float scale_y = dot(projectionMatrix * uy, uy);
  mat4 scale = mat4(scale_x, 0., 0., 0.,
                    0., scale_y, 0., 0.,
                    0., 0., 0., 0.,
                    0., 0., 0., 0.);
  // Extract translation from the matrices
  vec4 translation = projectionMatrix * modelViewMatrix * uw;
  // We don't use the full projection matrix because we don't want perspective
  gl_Position = translation + scale * modelViewMatrix * vec4(position, 0.);
}

</script>
