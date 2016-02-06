#! /usr/bin/env python

'''**********************************************************************
 Publite2 is a process manager which launches multiple Freeciv-web servers.
    Copyright (C) 2009-2015  The Freeciv-web project

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


import configparser
import json
import os, sys


print("parsing stdsounds.soundspec");
config = configparser.ConfigParser()
config.read("../../freeciv/freeciv/data/stdsounds.soundspec")

f = open('soundset_spec.js', 'w')
f.write("var soundset = {");


for key in config['files']:
  f.write("\"" + key + "\" : " );
  f.write(config['files'][key].replace("stdsounds/","").split(";")[0]);
  f.write(", ");

f.write("\"last\":null};");

