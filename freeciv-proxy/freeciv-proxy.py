#!/usr/bin/env python
# -*- coding: latin-1 -*-

''' 
 Freeciv - Copyright (C) 2011-2013 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
'''


from os import path as op
import time
from tornado import web, websocket, ioloop, httpserver
from debugging import *
import logging
from civcom import *
import json

ROOT = op.normpath(op.dirname(__file__))
PROXY_PORT = 8002;
WS_UPDATE_INTERVAL = 0.011;

civcoms = {};

class IndexHandler(web.RequestHandler):
    """Serves the Freeciv-proxy index page """
    def get(self):
        self.write("Freeciv-web websocket proxy, port: " + str(PROXY_PORT));


class StatusHandler(web.RequestHandler):
    """Serves the Freeciv-proxy status page, on the url:  /status """
    def get(self):
        self.write(get_debug_info(civcoms));


class WSHandler(websocket.WebSocketHandler):
    clients = []
    logger = logging.getLogger("freeciv-proxy");

    def open(self):
        self.clients.append(self)
        self.is_ready = False;

    def on_message(self, message):
        if (not self.is_ready):
          #called the first time the user connects.
          login_message = json.loads(message);
          self.username = login_message['username'];
          self.civserverport = login_message['port'];
          self.loginpacket = message;
          self.is_ready = True;
          self.civcom = self.get_civcom(self.username, self.civserverport, self);
          return; 
        
        # get the civcom instance which corresponds to this user.        
        self.civcom = self.get_civcom(self.username, self.civserverport, self);

        if (self.civcom == None):
          self.send("Error: Could not authenticate user.");
          return;

        try:
          # send JSON request to civserver.
          if not self.civcom.send_packets_to_civserver(message):
            if (logger.isEnabledFor(logging.INFO)):
              logger.info("Sending data to civserver failed.");
            self.write_message('Error: Civserver communication failure ')
            return;
 
        except:
          if (logger.isEnabledFor(logging.ERROR)):
            logger.error("Service unavailable: freeciv-web down.");
          self.write_message("Error: Service Unavailable. Something is wrong here ");



    def on_close(self):
       self.clients.remove(self)
       if hasattr(self, 'civcom') and self.civcom != None: 
          self.civcom.stopped = True;
          self.civcom.close_connection();
          if self.civcom.key in list(civcoms.keys()):
            del civcoms[self.civcom.key];



    # get the civcom instance which corresponds to the requested user.
    def get_civcom(self, username, civserverport, ws_connection):
      key = username + str(civserverport);
      if key not in list(civcoms.keys()):
        if (int(civserverport) < 5500): return None;
        civcom = CivCom(username, int(civserverport));
        civcom.connect_time = time.time();
        civcom.set_civwebserver(self);
        civcom.is_websocket_active = True;
        civcom.start();
        civcoms[key] = civcom;

        messenger = CivWsMessenger(civcom, ws_connection);
        messenger.start();

        time.sleep(0.4);
        return civcom;
      else:
        usrcivcom = civcoms[key];
        return usrcivcom;


class CivWsMessenger(Thread):
  """ This thread listens for messages from the civserver
      and sends the message to the WebSocket client. """

  def __init__ (self, civcom, ws_handler):
    Thread.__init__(self)
    self.daemon = True;
    self.civcom = civcom;
    self.ws_handler = ws_handler;

  def run(self):
    while 1:
      if (self.civcom.stopped or self.ws_handler == None): return;

      packet = self.civcom.get_send_result_string();
      if (packet != None and self.ws_handler != None):
        self.ws_handler.write_message(packet);
      time.sleep(WS_UPDATE_INTERVAL);



if __name__ == "__main__":
  try:
    print('Started Freeciv-proxy. Use Control-C to exit');
 
    if len(sys.argv) == 2:
      PROXY_PORT = int(sys.argv[1]);
    print(('port: ' + str(PROXY_PORT)));
 
    LOG_FILENAME = '/tmp/logging' +str(PROXY_PORT) + '.out'
    #logging.basicConfig(filename=LOG_FILENAME,level=logging.INFO)
    logging.basicConfig(level=logging.INFO)
    logger = logging.getLogger("freeciv-proxy");

    application = web.Application([
        (r'/civsocket', WSHandler),
        (r"/", IndexHandler),
        (r"/status", StatusHandler),
    ])

    http_server = httpserver.HTTPServer(application)
    http_server.listen(PROXY_PORT)
    ioloop.IOLoop.instance().start()

  except KeyboardInterrupt:
    print('Exiting...');





