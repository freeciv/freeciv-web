#!/usr/bin/env python
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

 

from webserver import *
from comcleanup import *
import logging
import sys

PROXY_PORT = 8082;

LOG_FILENAME = '/tmp/logging.out'

#logging.basicConfig(filename=LOG_FILENAME,level=logging.DEBUG)
logging.basicConfig(level=logging.WARNING)
logger = logging.getLogger("freeciv-proxy");

# start yappi performance profiler.
#import yappi
#yappi.start()


if len(sys.argv) == 2: 
  PROXY_PORT = int(sys.argv[1]);

print("Starting freeciv-proxy listening on port " + str(PROXY_PORT));

try:
  # init webserver
  server = CivWebServer(('', PROXY_PORT), WebserverHandler)
  
  # start cleanup service
  cleanup = ComCleanup(server.civcoms)
  cleanup.start();

  if (logger.isEnabledFor(logging.INFO)):
    logger.info('Started proxy HTTP server.');
  server.serve_forever();
except KeyboardInterrupt:
  if (logger.isEnabledFor(logging.ERROR)):
    logger.error('^C received, shutting down server');
  server.socket.close();

  #stats = yappi.get_stats(yappi.SORTTYPE_TTOTAL);
  #for stat in stats: 
  #  print(stat);
  #yappi.stop();

  sys.exit(0);


