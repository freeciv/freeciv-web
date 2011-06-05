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

var civwebserver_url_base = "/civ";

var websocket_enabled = true;

var error_shown = false;
var syncTimerId = -1;
var isWorking = false;

var clinet_last_send = 0;
var debug_client_speed_list = [];

var ws = null;

/****************************************************************************
  Initialized the network synchronization loop.
****************************************************************************/
function network_init()
{
  civwebserver_url = civwebserver_url_base + "?p=" + civserverport + "&u=" + username;
  
  websocket_enabled = $.jStorage.get("websocket_enabled");
 
  if (websocket_enabled) {
    network_websocket_init();

  } else {
    syncTimerId = setInterval("sync_civclient()", 800);
 
     $(document).ajaxComplete(function(){ 
       isWorking = false;
     });

  }
  
}



/****************************************************************************
  Initialized the Websocket connection.
****************************************************************************/
function network_websocket_init()
{
  ws = new io.Socket(window.location.hostname, {port: 8002, rememberTransport: false});
  ws.connect();

  ws.addEvent('connect', function() {
    /* first websocket packet contains username and port. */
    ws.send(username + ";" + civserverport);
  });

  ws.addEvent('message', function(data) {
     client_handle_packet(jQuery.parseJSON(data));
  });

}



/****************************************************************************
  Stops network sync.
****************************************************************************/
function network_stop()
{
  clearInterval(syncTimerId); 
}

/****************************************************************************
  Send a syncronization request to the server using POST.
****************************************************************************/
function sync_civclient()
{

  /* Prevent race conditions. */
  if (isWorking) return;
  isWorking = true;
  
  if (over_error_threshold()) return;
  
  var net_packet = [];
    
  var myJSONText = JSON.stringify(net_packet);
 
  /* Send main request for sync to server. */
  $.ajax({
      url: civwebserver_url,
      type: "POST",
      data: myJSONText,
      dataType: "json",
      success: client_handle_packet,
      error: function(XMLHttpRequest, textStatus, errorThrown){
        var error_msg = "Network error: " + textStatus + "\n"
                        + XMLHttpRequest.status + "\n"
                        + XMLHttpRequest.statusText + "\n\n"
                        + XMLHttpRequest.responseText + "\n\n";
      
         js_breakpad_report(error_msg, "clinet.js", 62); 
      }
      
   }
  );


}

/****************************************************************************
  Sends a request to the server, with a JSON packet.
****************************************************************************/
function send_request(packet_payload) 
{
  if (websocket_enabled) {
    ws.send(packet_payload);
  } else {
    $.post(civwebserver_url, packet_payload, client_handle_packet, "json");
  }
  if (debug_active) clinet_last_send = new Date().getTime();
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
