# -*- coding: latin-1 -*-

''' 
 Freeciv - Copyright (C) 2009-2013 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
'''

from threading import Thread
import time

WS_UPDATE_INTERVAL = 0.011;


class CivWsMessenger(Thread):
  """ This thread listens for messages from the civserver
      and sends the message to the WebSocket client. """

  def __init__ (self, civcoms):
    Thread.__init__(self)
    self.daemon = True;
    self.civcoms = civcoms;

  def run(self):
    while 1:
      for civcom in list(self.civcoms.values()):
        try:
          if (civcom.stopped or civcom.civwebserver == None): return;

          packet = civcom.get_send_result_string();
          if (packet != None and civcom.civwebserver != None):
            civcom.civwebserver.write_message(packet);
        except:
          print("Unexpected error:", sys.exc_info()[0]);
      time.sleep(WS_UPDATE_INTERVAL);


