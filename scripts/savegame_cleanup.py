#! /usr/bin/env python

from subprocess import call, PIPE
from os import *
from shutil import *
from threading import Thread
import time
import os
import sys

rootdir = "/var/lib/tomcat8/webapps/data/savegames/" 
scenario_list = ['british-isles-85x80-v2.80.sav.gz', 'iberian-peninsula-136x100-v1.0.sav.gz',
    'earth-160x90-v2.sav.gz', 'italy-100x100-v1.5.sav.gz',
    'earth-80x50-v3.sav.gz','japan-88x100-v1.3.sav.gz',
    'europe-200x100-v2.sav.gz','north_america_116x100-v1.2.sav.gz',
    'france-140x90-v2.sav.gz','tutorial.sav',
    'hagworld-120x60-v1.2.sav.gz']


keeptime = 60*60*24*365;
keeptime_pbem = 60*60*24*10;

for root, subFolders, files in os.walk(rootdir):
    for file in files:
        f = os.path.join(root,file)
        ftime = os.path.getmtime(f)
        curtime = time.time()
        difftime = curtime - ftime
        if difftime > keeptime and (file.endswith("sav.xz") or file.endswith(".sav")) and not file.startswith("pbem") and file not in scenario_list:
          print("rm file: " + file);
          os.unlink(f)
        if difftime > keeptime_pbem and (file.endswith(".xz") or file.endswith(".sav")) and file.startswith("pbem") and file not in scenario_list:
          print("rm file: " + file);
          os.unlink(f)

