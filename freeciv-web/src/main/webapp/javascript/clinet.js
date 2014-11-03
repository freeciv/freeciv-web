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

var freeciv_version = "+Freeciv.Web.Devel-2.6";

var ws = null;
var civserverport = null;

/****************************************************************************
  Initialized the Network communication, by requesting a valid server port.
****************************************************************************/
function network_init()
{

  if (!"WebSocket" in window) {
    swal("WebSockets not supported");
    return;
  }

  var civclient_request_url = "/civclientlauncher";
  if ($.getUrlVar('action') != null) civclient_request_url += "?action=" + $.getUrlVar('action');
  if ($.getUrlVar('action') == null && $.getUrlVar('civserverport') != null) civclient_request_url += "?";
  if ($.getUrlVar('civserverport') != null) civclient_request_url += "&civserverport=" + $.getUrlVar('civserverport');

  $.ajax({
   type: 'POST',
   url: civclient_request_url,
   success: function(data, textStatus, request){
       civserverport = request.getResponseHeader('port');
       var connect_result = request.getResponseHeader('result');
       if (civserverport != null && connect_result == "success") {
         websocket_init(); 
         load_game_check();

       } else {
         show_dialog_message("Network error", "Invalid server port. Error: " + connect_result);
       }
   },
   error: function (request, textStatus, errorThrown) {
	show_dialog_message("Network error", "Unable to communicate with civclientlauncher servlet . Error: " 
		+ textStatus + " " + errorThrown + " " + request.getResponseHeader('result')); 
   }
  });



}

/****************************************************************************
  Initialized the WebSocket connection.
****************************************************************************/
function websocket_init()
{
  var proxyport = 1000 + parseFloat(civserverport);
  var ws_protocol = ('https:' == document.location.protocol) ? "wss://" : "ws://";
  ws = new WebSocket(ws_protocol + window.location.hostname + "/civsocket/" + proxyport);

  ws.onopen = function () {
    var login_message = {"type":4, "username" : username,
    "capability": freeciv_version, "version_label": "-dev",
    "major_version" : 2, "minor_version" : 5, "patch_version" : 99,
    "port": civserverport};
    ws.send(JSON.stringify(login_message));
  };


  ws.onmessage = function (event) {
     if (typeof client_handle_packet !== 'undefined') {
       client_handle_packet(jQuery.parseJSON(event.data));
     } else {
       console.error("Error, freeciv-web not compiled correctly. Please "
             + "run sync.sh in freeciv-proxy correctly.");
     }
  };

  ws.onclose = function (event) {
   console.info("WebSocket connection closed, code+reason: " + event.code + ", " + event.reason); 
  };

  ws.onerror = function (evt) {
   show_dialog_message("Network error", "A problem occured with the " 
                       + document.location.protocol + " WebSocket connection to the server: " + ws.url); 
   console.error("WebSocket error: Unable to communicate with server using " 
                 + document.location.protocol + " WebSockets. Error: " + evt);
  };
}

/****************************************************************************
  Stops network sync.
****************************************************************************/
function network_stop()
{
  if (ws != null) ws.close();
  ws = null;
}

/****************************************************************************
  Sends a request to the server, with a JSON packet.
****************************************************************************/
function send_request(packet_payload) 
{
  if (ws != null) {
    ws.send(packet_payload);
  }

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
