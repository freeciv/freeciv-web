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


/****************************************************************************
 Init WebGL mapctrl.
****************************************************************************/
function init_webgl_mapctrl()
{
  document.addEventListener('mousemove', webglOnDocumentMouseMove, false );
  document.addEventListener('mousedown', webglOnDocumentMouseDown, false );
  document.addEventListener('keydown', webglOnDocumentKeyDown, false );
  document.addEventListener('keyup', webglOnDocumentKeyUp, false );
  window.addEventListener('resize', webglOnWindowResize, false );
}


/****************************************************************************
...
****************************************************************************/
function webglOnWindowResize() {

  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();

  maprenderer.setSize( window.innerWidth, window.innerHeight );

}

/****************************************************************************
...
****************************************************************************/
function webglOnDocumentMouseMove( event ) {

  event.preventDefault();
  mouse.set( ( event.clientX / window.innerWidth ) * 2 - 1, - ( event.clientY / window.innerHeight ) * 2 + 1 );

  raycaster.setFromCamera( mouse, camera );

  var intersects = raycaster.intersectObjects( objects );

  if ( intersects.length > 0 ) {

    var intersect = intersects[ 0 ];

    rollOverMesh.position.copy( intersect.point ).add( intersect.face.normal );
    rollOverMesh.position.divideScalar( 50 ).floor().multiplyScalar( 50 ).addScalar( 25 );

  }

}

/****************************************************************************
...
****************************************************************************/
function webglOnDocumentMouseDown( event ) {

  event.preventDefault();

  mouse.set( ( event.clientX / window.innerWidth ) * 2 - 1, - ( event.clientY / window.innerHeight ) * 2 + 1 );

  raycaster.setFromCamera( mouse, camera );

  var intersects = raycaster.intersectObjects( scene.children );

  if ( intersects.length > 0 ) {

    var intersect = intersects[ 0 ];
    camera_look_at(intersect.point.x, 0, intersect.point.z);
    //console.log("X: " + intersect.point.x + ", Y: " + intersect.point.z);
  }

}

/****************************************************************************
...
****************************************************************************/
function webglOnDocumentKeyDown( event ) {

  switch( event.keyCode ) {
     case 16: isShiftDown = true; break;

  }

}

/****************************************************************************
...
****************************************************************************/
function webglOnDocumentKeyUp( event ) {
  switch ( event.keyCode ) {
    case 16: isShiftDown = false; break;

  }

}


