#!/usr/bin/env python
# -*- coding: latin-1 -*-

''' 
 Freeciv - Copyright (C) 2011 - Andreas Røsdal   andrearo@pvv.ntnu.no
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

from tornado import web

from tornadio2 import SocketConnection, TornadioRouter, SocketServer, event

from threading import Thread;

from civcom import *
from debugging import *
import logging


ROOT = op.normpath(op.dirname(__file__))
WS_UPDATE_INTERVAL = 0.022;
PROXY_PORT = 8002;

civcoms = {};

class IndexHandler(web.RequestHandler):
    """Serves the Freeciv-proxy index page """
    def get(self):
        self.write("Freeciv-web websocket proxy");


class StatusHandler(web.RequestHandler):
    """Serves the Freeciv-proxy status page, on the url:  /status """
    def initialize(self, router):
        self.router = router;

    def get(self):
        self.write(get_debug_info(civcoms, self.router));

class CivConnection(SocketConnection):
    """ Serves the Freeciv WebSocket service. """
    participants = set()

    def on_open(self, request):
	self.ip = self.user_id = self.session.remote_ip;
        self.participants.add(self);
	self.is_ready = False;

    def on_message(self, message):
        if (not self.is_ready):
          #called the first time the user connects.
	  auth = message.split(";");
	  self.username = auth[0];
	  self.civserverport = auth[1];
	  self.is_ready = True;
          self.civcom = self.get_civcom(self.username, self.civserverport, self.ip, self);
	  return;
        
        # get the civcom instance which corresponds to this user.        
        self.civcom = self.get_civcom(self.username, self.civserverport, self.ip, self);

        if (self.civcom == None):
          self.send("Error: Could not authenticate user.");
          return;

        try:
          # send JSON request to civserver.
          if not self.civcom.send_packets_to_civserver(message):
            if (logger.isEnabledFor(logging.INFO)):
              logger.info("Sending data to civserver failed.");
	    self.send('Error: Civserver communication failure ')
            return;
 
        except Exception, e:
          if (logger.isEnabledFor(logging.ERROR)):
            logger.error(e);
	  self.send("Error: Service Unavailable. Something is wrong here ");


    def on_close(self):
        self.participants.remove(self)
	if self.civcom != None:
          self.civcom.stopped = True;
          self.civcom.close_connection();
	  if self.civcom.key in civcoms.keys():
            del civcoms[self.civcom.key];


    # get the civcom instance which corresponds to the requested user.
    def get_civcom(self, username, civserverport, client_ip, ws_connection):
      key = username + str(civserverport);
      if key not in civcoms.keys():
        civcom = CivCom(username, int(civserverport));
        civcom.client_ip = client_ip;
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
        if (usrcivcom.client_ip != client_ip):
          logger.error("Unauthorized connection from client IP: " + client_ip);
          return None;
        return usrcivcom;




class CivWsMessenger(Thread):
  """ This thread listens for messages from the civserver
      and sends the message to the WebSocket client. """

  def __init__ (self, civcom, ws_connection):
    Thread.__init__(self)
    self.daemon = True;
    self.civcom = civcom;
    self.ws_connection = ws_connection;

  def run(self):
    while 1:
      if (self.civcom.stopped): return;

      packet = self.civcom.get_send_result_string();
      if (len(packet) > 4):
        self.ws_connection.send(packet);
      time.sleep(WS_UPDATE_INTERVAL);




if __name__ == "__main__":
  try:
    print 'Started Freeciv-proxy. Use Control-C to exit'
 
    if len(sys.argv) == 2:
      PROXY_PORT = int(sys.argv[1]);
    print 'port: ' + str(PROXY_PORT);
 
    LOG_FILENAME = '/tmp/logging' +str(PROXY_PORT) + '.out'
    #logging.basicConfig(filename=LOG_FILENAME,level=logging.INFO)
    logging.basicConfig(level=logging.INFO)
    logger = logging.getLogger("freeciv-proxy");

    CivRouter = TornadioRouter(CivConnection,
                               dict(enabled_protocols = ['websocket']));

    application = web.Application( 
        CivRouter.apply_routes([(r"/", IndexHandler),
                                (r"/status", StatusHandler, dict(router=CivRouter))]),
        socket_io_port = PROXY_PORT
    )

    # Create and start tornadio server
    SocketServer(application);
  except KeyboardInterrupt:
    print('Exiting...');





