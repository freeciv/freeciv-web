from threading import Thread
from subprocess import call, PIPE
from shutil import *
from os import path
import sys
from time import gmtime, strftime
import time

def _get_relative_path(filepath):
    return path.join(path.dirname(__file__), filepath)

# The Civlauncher class launches a new instance of a Freeciv-web server in a
# separate thread and restarts the process when the game ends.
class Civlauncher(Thread):

    def __init__ (self, gametype, scripttype, new_port, metahostpath, savesdir):
        Thread.__init__(self)
        self.new_port = new_port;
        self.gametype = gametype;
        self.scripttype = scripttype;
        self.metahostpath = metahostpath;
        self.savesdir = savesdir;
        self.started_time = strftime("%Y-%m-%d %H:%M:%S", gmtime());
        self.num_start = 0;
        self.num_error = 0;

    def run(self):
        while 1:
            try:
                print("Start freeciv-web on port " + str(self.new_port) +
                      " and freeciv-proxy on port " + str(1000 + self.new_port) + ".");
                retcode = call(_get_relative_path("init-freeciv-web.sh") + " "
                               + self.savesdir + " "
                               + str(self.new_port) + " "
                               + str(1000 + self.new_port) + " "
                               + self.metahostpath + " "
                               + self.gametype + " "
                               + self.scripttype, shell=True)
                self.num_start += 1;
                if retcode > 0:
                    print("Freeciv-web port " + str(self.new_port) + " was terminated by signal " + str(retcode))
                    self.num_error += 1;
                else:
                    print("Freeciv-web port " + str(self.new_port) + " returned " + str(retcode))
            except OSError as e:
                print("Execution failed:", e, file=sys.stderr)
                self.num_error += 1;
            time.sleep(5)

