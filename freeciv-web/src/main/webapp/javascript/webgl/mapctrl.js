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

  var intersects = raycaster.intersectObjects( objects );

  if ( intersects.length > 0 ) {

    var intersect = intersects[ 0 ];

    // delete cube

    if ( isShiftDown ) {

      if ( intersect.object != plane ) {

        scene.remove( intersect.object );

        objects.splice( objects.indexOf( intersect.object ), 1 );

      }

        // create cube

    } else {

        var voxel = new THREE.Mesh( cubeGeo, cubeMaterial );
        voxel.position.copy( intersect.point ).add( intersect.face.normal );
        voxel.position.divideScalar( 50 ).floor().multiplyScalar( 50 ).addScalar( 25 );
        scene.add( voxel );

        objects.push( voxel );
    }

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


