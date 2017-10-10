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


/****************************************************************************
 Update a 3d model by uploading a new file.
****************************************************************************/
function update_webgl_model()
{
  // reset dialog page.
  $("#upload_dialog").remove();
  $("<div id='upload_dialog'></div>").appendTo("div#game_page");
  var html = "Upload a .glb file: <input type='file' id='model_file'><br><br>Model name: <input type='text' id='model_name' value='Settlers'><br><br>";

  $("#upload_dialog").html(html);
  $("#upload_dialog").attr("title", "Upload a glTF (.glb) file to update a 3d model.");
  $("#upload_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "55%" : "40%",
			buttons: {
                                "Cancel" : function() {
                                  $("#upload_dialog").dialog('close');
                                },
                                "Upload": function() {
                                  handle_3d_model_upload();
                                  $("#upload_dialog").dialog('close');
   				}
			}
		});
  $("#upload_dialog").dialog('open');
}

/**************************************************************************
...
**************************************************************************/
function handle_3d_model_upload()
{
  var fileInput = document.getElementById('model_file');
  var file = fileInput.files[0];

  if (file == null) {
    swal("Please upload a .glb file!");
    return;
  }

  if (!(window.FileReader)) {
    swal("Uploading files not supported");
    return;
  }

  var extension = file.name.substring(file.name.lastIndexOf('.'));
  console.log("Loading 3d model of type: " + file.type + " with extention " + extension);

  if (extension == '.glb') {
    var reader = new FileReader();
    reader.onload = function(e) {
      update_3d_model_from_file(reader.result);
    }
    reader.readAsArrayBuffer(file);
  } else {
    swal("Image file " + file.name + "  not supported: " + file.type);
    console.error("Image file not supported: " + file.type);
  }

  $("#upload_dialog").dialog('close');
  $("#dialog").dialog('close');

}


/**************************************************************************
...
**************************************************************************/
function update_3d_model_from_file(model_data) {
  var model_name_to_update = $("#model_name").val();

  var glTFLoader = new THREE.GLTFLoader();

  glTFLoader.parse( model_data, '', function(data) {
    var model = data.scene;
    model.traverse((node) => {
      if (node.isMesh) {
        node.material.flatShading = false;
        node.material.side = THREE.DoubleSide;
        node.material.needsUpdate = true;
        node.geometry.computeVertexNormals();
      }
    });


    model.scale.x = model.scale.y = model.scale.z = 11;
    webgl_models[model_name_to_update] = model;
    swal(model_name_to_update + " 3d model updated!");

   });

  $("#upload_dialog").dialog('close');
  $("#dialog").dialog('close');

}