#! /usr/bin/env python
# Publite2 is a process manager which launches multiple Freeciv-web servers
# depending on demand reported by the Metaserver. 

from subprocess import call, PIPE
from os import *
from shutil import *
from threading import Thread
import sys
import time
import http.client

port = 5556
metahost = "localhost:8080"
metapath =  "/meta/metaserver.php"
statuspath =  "/meta/status.php"
logdir = "/tmp/"
savesdir = "~/freeciv-build/resin/webapps/ROOT/savegames/"
game_types = ["singleplayer", "multiplayer"]
pubscript = "pubscript_"
server_capacity = 3
server_limit = 200  
metachecker_interval = 60

# The Metachecker class connects to the Freeciv-web metaserver, gets the number of available
# Freeciv-web server instances, and launches new servers if needed.
class metachecker():
    def __init__(self):
      pass;

    def check(self):
      global port;
      while 1:
        try:
          time.sleep(1);
          conn = http.client.HTTPConnection(metahost);
          conn.request("GET", statuspath);
          r1 = conn.getresponse();
          if (r1.status == 200):
            html_doc = r1.read()
            meta_status = html_doc.decode('ascii').split(";");
            if (len(meta_status) == 4):
              total_servers = int(meta_status[1]);
              single_servers = int(meta_status[2]);
              multi_servers = int(meta_status[3]);
              print("status=[total=" + str(total_servers) + ",single=" + str(single_servers) 
                    + ",multi=" + str(multi_servers) + "]");

              while (single_servers < server_capacity and total_servers <= server_limit):
                new_server = civserverproc(game_types[0], port);
                new_server.start();
                port += 1;
                total_servers += 1;
                single_servers += 1;
 
              while (multi_servers < server_capacity and total_servers <= server_limit):
                new_server = civserverproc(game_types[1], port)
                new_server.start();
                port += 1;
                total_servers += 1;
                multi_servers += 1;
          else:
            print("Error: Invalid metaserver status");

          time.sleep(metachecker_interval);
        except Exception as e:
          print("Error: Publite2 is unable to connect to Freeciv-web metaserver on http://" 
                + metahost + metapath + ", error" + str(e));
        finally:
          conn.close();

# The Civserverproc class launches a new instance of a Freeciv-web server in a 
# separate thread and restarts the process when the game ends.
class civserverproc(Thread):

    def __init__ (self, gametype, port):
        Thread.__init__(self)
        self.port = port;
        self.gametype = gametype;

    def run(self):
        while 1:
            try:
                print("Starting new Freeciv-web server at port " + str(port));
                retcode = call("freeciv-web --port " + str(self.port) + " -q 20 -e " +
                               " -m -M http://" + metahost + metapath  + " --type \"" + self.gametype +
                               "\" --read " + pubscript + self.gametype + ".serv" + 
                               " --log " + logdir + "fcweb-" + str(self.port) + ".log " +
                               "--saves " + savesdir, shell=True)
                if retcode < 0:
                    print("Freeciv-web was terminated by signal", -retcode, file=sys.stderr)
                else:
                    print("Freeciv-web returned", retcode, file=sys.stderr)
            except OSError as e:
                print("Execution failed:", e, file=sys.stderr)
            time.sleep(5)

#perform a test-request to the Metaserver
try:
  conn = http.client.HTTPConnection(metahost);
  conn.request("GET", statuspath);
  r1 = conn.getresponse();
except Exception as e:
  print("Error: Publite2 is unable to connect to Freeciv-web metaserver on http://" 
        + metahost + metapath + ", error" + str(e));
  sys.exit(1)
finally:
  conn.close();

for type in game_types:
  for srv_num in range(server_capacity):
    new_server = civserverproc(type, port)
    new_server.start();
    port += 1;

print("Publite2 started!");
time.sleep(10);
metachecker().check();

