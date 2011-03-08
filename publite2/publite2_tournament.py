#! /usr/bin/env python

from subprocess import call, PIPE
from os import *
from threading import Thread
import sys
import time

port = 5665
# FIXME: This must be the hostname and port of resin server.
metaserver = "http://localhost:8080/freecivmetaserve/metaserver.php"
logdir = "/tmp/"
pubscript = "pubscript_tournament.serv"
servers_count = 5


class civserverproc(Thread):

    def __init__ (self, port):
        Thread.__init__(self)
        self.port = port

    def run(self):
        while 1:
            try:
                retcode = call("civserver --ranked --port " + str(self.port) + " -q 20 -e " 
                        + " -m -M " + metaserver  + " --read " + pubscript +  " --log " + logdir + "civserver-" + str(self.port) + ".log", shell=True)
                if retcode < 0:
                    print >>sys.stderr, "Civserver was terminated by signal", -retcode
                else:
                    print >>sys.stderr, "Civserver returned", retcode
            except OSError, e:
                print >>sys.stderr, "Execution failed:", e

            time.sleep(10)


for i in range(servers_count):
    current = civserverproc(port)
    port += 1
    current.start()

print("Civservers done!")
