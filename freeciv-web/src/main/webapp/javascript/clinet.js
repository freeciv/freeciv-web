/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/


var error_shown = false;
var syncTimerId = -1;
var isWorking = false;

var clinet_last_send = 0;
var debug_client_speed_list = [];

var freeciv_version = "+Freeciv.Web.Devel-2.6-2013.Jul.21";

var ws = null;

/****************************************************************************
  Initialized the Network communication with Websockets.
****************************************************************************/
function network_init()
{

  if (!"WebSocket" in window) {
    alert("WebSockets not supported");
    return;
  }

  ws = new WebSocket("ws://" + window.location.hostname + "/civsocket");

  ws.onopen = function () {
    var login_message = {"type":4, "username" : username,
    "capability": freeciv_version, "version_label": "-dev",
    "major_version" : 2, "minor_version" : 4, "patch_version" : 99,
    "port": civserverport};
    ws.send(JSON.stringify(login_message));
  };


  ws.onmessage = function (event) {
     if (typeof client_handle_packet !== 'undefined') {
       client_handle_packet(jQuery.parseJSON(event.data));
     } else {
       alert("Error, freeciv-web not compiled correctly. Please "
             + "run sync.sh in freeciv-proxy correctly.");
     }
  };

  ws.onclose = function (event) {
   console.log("WebSocket connection closed."); 
  };

  ws.onerror = function (errpr) {
   show_dialog_message("Network error", errpr); 
  };




}



/****************************************************************************
  Stops network sync.
****************************************************************************/
function network_stop()
{
  ws.close();
}

/****************************************************************************
  Sends a request to the server, with a JSON packet.
****************************************************************************/
function send_request(packet_payload) 
{
  ws.send(packet_payload);

  if (debug_active) {
    clinet_last_send = new Date().getTime();
  }
}


/****************************************************************************
...
****************************************************************************/
function clinet_debug_collect()
{
  var time_elapsed = new Date().getTime() - clinet_last_send;
  debug_client_speed_list.push(time_elapsed);
  clinet_last_send = new Date().getTime();
}
