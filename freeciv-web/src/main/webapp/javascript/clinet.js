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


function network_init()
{
  civwebserver_url = civwebserver_url_base + "?p=" + civserverport + "&h=" + civserverhost + "&u=" + username;

  if (jQuery.browser.msie) {
    syncTimerId = setInterval("sync_civclient()", 200);
  } else {
    syncTimerId = setInterval("sync_civclient()", 30);
  }
  
 
 $(document).ajaxComplete(function(){ 
   isWorking = false;
 });
 
 
  
}

function network_stop()
{
  clearInterval(syncTimerId); 
}

function sync_civclient()
{

  /* Prevent race conditions. */
  if (isWorking) return;
  isWorking = true;
  
  if (over_error_threshold()) return;
  
  var net_packet = [];
    
  var myJSONText = JSON.stringify(net_packet);
  
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
)


}


function send_request(packet_payload) 
{
  $.post(civwebserver_url, packet_payload, client_handle_packet, "json");
}


