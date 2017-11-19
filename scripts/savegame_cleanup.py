#! /usr/bin/env python

from subprocess import call, PIPE
from os import *
from shutil import *
from threading import Thread
import time
import os
import sys

rootdir = "/var/lib/tomcat8/webapps/data/savegames/" 
scenario_list = ['british-isles.sav', 'iberian-peninsula.sav',
    'earth-large.sav', 'italy.sav',
    'earth-small.sav','japan.sav',
    'europe.sav','north_america.sav',
    'france.sav','tutorial.sav',
    'hagworld.sav']


keeptime = 60*60*24*365;
keeptime_pbem = 60*60*24*10;
keeptime_autosave = 60*60*24*5;

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
        if difftime > keeptime_autosave and file.endswith("auto.sav.xz"):
          print("rm file: " + file);
          os.unlink(f)
        if difftime > keeptime_autosave and file.startswith("freeciv-earth-savegame"):
          print("rm file: " + file);
          os.unlink(f)          
