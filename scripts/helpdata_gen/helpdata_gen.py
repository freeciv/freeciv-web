#!/usr/bin/python
# -*- coding: iso-8859-1 -*-
''' 
 Freeciv - Copyright (C) 2009-2014 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
'''

import os, sys

import configparser
import json
import re

def removeComments(string):
    string = re.sub(re.compile("/\*.*?\*/",re.DOTALL ) ,"" ,string);
    string = re.sub(re.compile("//.*?\n" ) ,"" ,string); 
    string = re.sub(re.compile("([a-zA-Z]);" ) ,"\g<1>," ,string); 
    string = re.sub(re.compile(";.*?\n" ) ,"" ,string); 
    return string

def config_read(file):
  print(("Parsing " + file ));
  config = configparser.ConfigParser(strict=False)
  with open (file, "r") as myfile:
    config_text=myfile.read();
    # These changes are required so that the Python config parser can read Freeciv spec files.
    config_text = config_text.replace("*include", "#*include");
    config_text = config_text.replace("\\\"", "'");
    config_text = config_text.replace("\n\"", "\n \"");
    config_text = config_text.replace("\n}", "\n }");
    config_text = config_text.replace("%", "%%");
    config_text = config_text.replace("\n1", "\n 1");
    config_text = removeComments(config_text);
    config_text = config_text.replace("\\\n", "");
    config_text = config_text.replace("\n_(\"", "_(\"");
    config_text = config_text.replace("_(\"", "");
    config_text = config_text.replace("\"),", "<br><br>");
    config_text = config_text.replace("\")", "");
    config_text = config_text.replace("\\n", "<br>");

    #print(config_text);
    config.read_string(config_text);
    #print((config.sections()));
    return config;




parser = config_read("../freeciv/freeciv/data/helpdata.txt");
thedict = {}
for section in parser.sections():
    thedict[section] = {}
    for key, val in parser.items(section):
        # skip these hidden help sections, since they are not in use.
        if (section in ["help_connecting", "help_languages", "help_governor", 
    "help_chatline", "help_about", "help_worklist_editor", "help_copying"]): continue;
        thedict[section][key] = val

f = open('freeciv-helpdata.js', 'w')

f.write("var helpdata_order = " + json.dumps(parser.sections()) + ";  var helpdata = " + json.dumps(thedict, separators=(',',':')) + ";");


print("done extracting helpdata.");
