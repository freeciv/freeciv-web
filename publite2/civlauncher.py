from threading import Thread
from subprocess import call, PIPE
from shutil import *
import sys
from time import gmtime, strftime
import time

pubscript = "pubscript_"
logdir = "../logs/"

# The Civlauncher class launches a new instance of a Freeciv-web server in a 
# separate thread and restarts the process when the game ends.
class Civlauncher(Thread):

    def __init__ (self, gametype, new_port, metahostpath, savesdir):
        Thread.__init__(self)
        self.new_port = new_port;
        self.gametype = gametype;
        self.metahostpath = metahostpath;
        self.savesdir = savesdir;
        self.started_time = strftime("%Y-%m-%d %H:%M:%S", gmtime());
        self.num_start = 0;
        self.num_error = 0;

    def run(self):
        while 1:
            try:
                print("Starting new Freeciv-web server at port " + str(self.new_port) + 
                      " and Freeciv-proxy server at port " + str(1000 + self.new_port) + ".");
                retcode = call("ulimit -t 10000 && export FREECIV_SAVE_PATH=\"" + self.savesdir + "\";" +
                               "python3.4 ../freeciv-proxy/freeciv-proxy.py " + 
                               str(1000 + self.new_port) + " > " + logdir + "freeciv-proxy-" + 
                               str(1000 + self.new_port) +".log 2>&1 " + 
                               "& proxy_pid=$! && " +
                               "freeciv-web --port " + str(self.new_port) + 
                               " -q 20 --Announce none -e " +
                               " -m -M http://" + self.metahostpath  + " --type \"" + self.gametype +
                               "\" --read " + pubscript + self.gametype + ".serv" + 
                               " --log " + logdir + "fcweb-" + str(self.new_port) + ".log " +
                               "--saves " + self.savesdir + " > " +  logdir + "freeciv-web-" +
                               str(self.new_port) + ".log  2>&1 " +
                               " ; rc=$?; kill -9 $proxy_pid; exit $rc", shell=True)
                self.num_start += 1;
                if retcode > 0:
                    print("Freeciv-web port " + str(self.new_port) + " was terminated by signal", 
                          -retcode, file=sys.stderr)
                    self.num_error += 1;
                else:
                    print("Freeciv-web port " + str(self.new_port) + " returned", 
                          retcode, file=sys.stderr)
            except OSError as e:
                print("Execution failed:", e, file=sys.stderr)
                self.num_error += 1;
            time.sleep(5)

