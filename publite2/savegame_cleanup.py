#! /usr/bin/env python

from subprocess import call, PIPE
from os import *
from shutil import *
from threading import Thread
import time
import os
import sys

rootdir = "/mnt/savegames" 
keeptime = 60 * 60 * 24 * 30;

for root, subFolders, files in os.walk(rootdir):
    for file in files:
        f = os.path.join(root,file)
	ftime = os.path.getmtime(f)
        curtime = time.time()
        difftime = curtime - ftime
        if difftime > keeptime and (file.endswith("sav.gz") or file.endswith("sav.bz2")):
          print("rm file: " + file);
          os.unlink(f)

