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

var error_shown = false;
var syncTimerId = -1;
var isWorking = false;

var clinet_last_send = 0;
var debug_client_speed_list = [];

var ws = null;

/****************************************************************************
  Initialized the Network communication with Websockets.
****************************************************************************/
function network_init()
{
  ws = new io.connect('http://' + window.location.hostname);

  ws.on('connect', function() {
    /* first websocket packet contains username and port. */
    ws.send(username + ";" + civserverport);
  });

  ws.on('message', function(data) {
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
