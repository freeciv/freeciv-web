#! /usr/bin/env python

'''**********************************************************************
 Publite2 is a process manager which launches multiple Freeciv-web servers.
    Copyright (C) 2009-2017  The Freeciv-web project

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************'''


from os import *

import sys
import time
import http.client
import configparser
from pubstatus import *
from civlauncher import Civlauncher
from os.path import dirname, abspath
from os import environ, chdir

metahost = "localhost"
metaport = 80
metapath =  "/meta/metaserver"
statuspath =  "/meta/status"
settings_file = "settings.ini"
game_types = ["singleplayer", "multiplayer", "pbem"]

metachecker_interval = 40
port = int(environ['PUBLITE2_PORT'])

# The Metachecker class connects to the Freeciv-web metaserver, gets the number of available
# Freeciv-web server instances, and launches new servers if needed.
class metachecker():
    def __init__(self):
      self.server_list = []
      self.server_capacity_single = int(environ['FREECIV_WEB_SERVER_CAPACITY_SINGLE'])
      self.server_capacity_multi = int(environ['FREECIV_WEB_SERVER_CAPACITY_MULTI'])
      self.server_capacity_pbem = int(environ['FREECIV_WEB_SERVER_CAPACITY_PBEM'])

      self.server_limit = int(environ['FREECIV_WEB_SERVER_LIMIT'])
      self.savesdir = environ['FREECIV_WEB_SAVE_GAME_DIRECTORY']

      self.start_longturn = bool(environ['FREECIV_WEB_START_LONGTURN'])

      longturn_ports_str = environ['FREECIV_WEB_LONGTURN_SERVER_PORTS'].split(" ")
      self.longturn_ports_list = [int(e) if e.isdigit() else e for e in longturn_ports_str]

      self.check_count = 0;
      self.total = 0;
      self.single = 0;
      self.multi = 0;
      self.pbem = 0;
      self.html_doc = "-";
      self.last_http_status = -1;
      s = PubStatus(self)
      s.start();

    def check(self, port):
      while 1:
        try:
          time.sleep(1);
          conn = http.client.HTTPConnection(metahost, metaport);
          conn.request("GET", statuspath);
          r1 = conn.getresponse();
          self.check_count += 1;
          self.last_http_status = r1.status;
          if (r1.status == 200):
            self.html_doc = r1.read()
            meta_status = self.html_doc.decode('ascii').split(";");
            if (len(meta_status) == 5):
              self.total = int(meta_status[1]);
              self.single = int(meta_status[2]);
              self.multi = int(meta_status[3]);
              self.pbem = int(meta_status[4]);

              fork_bomb_preventer = (self.total == 0 and self.server_limit < len(self.server_list))
              if fork_bomb_preventer:
                print("Error: Have tried to start more than " + str(self.server_limit)
                      + " servers (the server limit) but according to the"
                      + " metaserver it has found none.");

              while (self.single < self.server_capacity_single
                     and self.total <= self.server_limit
                     and not fork_bomb_preventer):
                time.sleep(1)
                new_server = Civlauncher(game_types[0], game_types[0], port, metahost + ":" + str(metaport) + metapath, self.savesdir);
                self.server_list.append(new_server);
                new_server.start();
                port += 1;
                self.total += 1;
                self.single += 1;
 
              while (self.multi < self.server_capacity_multi
                     and self.total <= self.server_limit
                     and not fork_bomb_preventer):
                time.sleep(1)
                new_server = Civlauncher(game_types[1], game_types[1], port, metahost + ":" + str(metaport) + metapath, self.savesdir)
                self.server_list.append(new_server);
                new_server.start();
                port += 1;
                self.total += 1;
                self.multi += 1;

              while (self.pbem < self.server_capacity_pbem
                     and self.total <= self.server_limit
                     and not fork_bomb_preventer):
                time.sleep(1)
                new_server = Civlauncher(game_types[2], game_types[2], port, metahost + ":" + str(metaport) + metapath, self.savesdir)
                self.server_list.append(new_server);
                new_server.start();
                port += 1;
                self.total += 1;
                self.pbem += 1;


          else:
            print("Error: Invalid metaserver status");

        except Exception as e:
          self.html_doc = ("Error: Publite2 is unable to connect to Freeciv-web metaserver on http://" + 
                          metahost + metapath + ", error" + str(e));
          print(self.html_doc);
        finally:
          conn.close();
          time.sleep(metachecker_interval);


if __name__ == '__main__':
  chdir(dirname(abspath(__file__)))
  #perform a test-request to the Metaserver
  try:
    conn = http.client.HTTPConnection(metahost, metaport);
    conn.request("GET", statuspath);
    r1 = conn.getresponse();
  except Exception as e:
    print("Error: Publite2 is unable to connect to Freeciv-web metaserver on http://" 
          + metahost + metapath + ", error" + str(e));
    sys.exit(1)
  finally:
    conn.close();

  time.sleep(16)
  # start the initial Freeciv-web servers
  mc = metachecker()
  for type in game_types:
    new_server = Civlauncher(type, type, port, metahost + ":" + str(metaport) + metapath, mc.savesdir)
    mc.server_list.append(new_server);
    new_server.start();
    port += 1;
  
  if mc.start_longturn:
    # start LongTurn games
    for i in mc.longturn_ports_list:  
      new_server = Civlauncher("longturn", "longturn_" + str(i), i, metahost + ":" + str(metaport) + metapath, mc.savesdir)
      mc.server_list.append(new_server);
      new_server.start();
      port += 1;
  else:    
    port += len(mc.longturn_ports_list);

  print("Publite2 started!");
  time.sleep(20);
  mc.check(port);
