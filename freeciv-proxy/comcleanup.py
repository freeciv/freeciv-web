# -*- coding: latin-1 -*-

''' 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
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
import traceback
import logging
import time

logger = logging.getLogger("freeciv-proxy");

PINGLIMIT = 20;
CLEANUP_TIMER = 8;

class ComCleanup(Thread):

  

  def __init__ (self, civcoms):
    Thread.__init__(self)
    self.civcoms = civcoms;


  # This thread checks that all civcom instances are vaild,
  # and removed the ones which are not.
  def run(self):
    while 1:
        time.sleep(CLEANUP_TIMER);
        try:
          for key in self.civcoms.keys():
              civcom = self.civcoms[key];        
              # if ping limit exceeded, close connection and remove civcom.
              if (PINGLIMIT < time.time() - civcom.pingstamp):
                civcom.stopped = True;
                civcom.close_connection();
        except:
          if (logger.isEnabledFor(logging.ERROR)):
            logging.error("Exception in ComCleanup");
          traceback.print_exc()

