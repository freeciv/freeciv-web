<!-- GLSL Fragment Shader for Freeciv-web -->
<script id="darkness_fragment_shh" type="x-shader/x-fragment">
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
varying vec2 vUv;
varying vec3 vPosition;

float darkness_high_height = 63.0;
float darkness_higher_height = 85.0;


void main(void)
{
  float darkness_alpha = -0.45 + vNormal.y * 1.8;

  if (vPosition.y > darkness_high_height) darkness_alpha += ((vPosition.y - darkness_high_height) / 50.0);
  if (vPosition.y > darkness_higher_height) darkness_alpha += ((vPosition.y - darkness_higher_height) / 50.0);

  darkness_alpha = min(darkness_alpha,1.0);
  darkness_alpha = max(darkness_alpha,0.0);
  if (darkness_alpha < 0.33) darkness_alpha = 0.0;

  gl_FragColor = vec4(0.0, 0.0, 0.0, darkness_alpha);

}

</script>
