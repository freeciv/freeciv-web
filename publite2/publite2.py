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
import configparser

metahost = "localhost:8080"
metapath =  "/meta/metaserver.php"
statuspath =  "/meta/status.php"
logdir = "/tmp/"

game_types = ["singleplayer", "multiplayer"]
pubscript = "pubscript_"
metachecker_interval = 60

savesdir = "~/freeciv-build/freeciv-web/resin/webapps/ROOT/savegames/"
if path.isdir("/vagrant"): savesdir = "/vagrant/resin/webapps/ROOT/savegames/"

settings = configparser.ConfigParser()
settings.read("settings.ini")

server_capacity = int(settings["Resource usage"]["server_capacity"])
server_limit = int(settings["Resource usage"]["server_limit"])

# The Metachecker class connects to the Freeciv-web metaserver, gets the number of available
# Freeciv-web server instances, and launches new servers if needed.
class metachecker():
    def __init__(self):
      pass;

    def check(self, port):
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
                    + ",multi=" + str(multi_servers) + ",port=" + str(port) + "]");

              fork_bomb_preventer = (total_servers == 0
                                     and server_limit < civserverproc.existing_instances)
              if fork_bomb_preventer:
                print("Error: Have tried to start more than " + str(server_limit)
                      + " servers (the server limit) but according to the"
                      + " metaserver it has found none.");

              while (single_servers < server_capacity
                     and total_servers <= server_limit
                     and not fork_bomb_preventer):
                time.sleep(1)
                new_server = civserverproc(game_types[0], port);
                new_server.start();
                port += 1;
                total_servers += 1;
                single_servers += 1;
 
              while (multi_servers < server_capacity
                     and total_servers <= server_limit
                     and not fork_bomb_preventer):
                time.sleep(1)
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
    existing_instances = 0

    def __init__ (self, gametype, new_port):
        Thread.__init__(self)
        self.new_port = new_port;
        self.gametype = gametype;
        civserverproc.existing_instances = civserverproc.existing_instances + 1

    def run(self):
        while 1:
            try:
                print("Starting new Freeciv-web server at port " + str(self.new_port) + 
                      " and Freeciv-proxy server at port " + str(1000 + self.new_port) + ".");
                retcode = call("python3.4 ../freeciv-proxy/freeciv-proxy.py " + str(1000 + self.new_port) + " & proxy_pid=$! && " +
                               "freeciv-web --port " + str(self.new_port) + " -q 20 --Announce none -e " +
                               " -m -M http://" + metahost + metapath  + " --type \"" + self.gametype +
                               "\" --read " + pubscript + self.gametype + ".serv" + 
                               " --log " + logdir + "fcweb-" + str(self.new_port) + ".log " +
                               "--saves " + savesdir + " ; kill -9 $proxy_pid", shell=True)
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

port = 6000
for type in game_types:
  for srv_num in range(server_capacity):
    new_server = civserverproc(type, port)
    new_server.start();
    port += 1;

print("Publite2 started!");
time.sleep(10);
metachecker().check(port);

