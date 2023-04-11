#!/usr/bin/env python3

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


from os import path
import argparse
import configparser

parser = argparse.ArgumentParser(
  description='Generate .js sound data based on freeciv data')
parser.add_argument('-f', '--freeciv', required=True, help='path to (original) freeciv project')
parser.add_argument('-o', '--outdir', required=True, help='path to webapp output directory')
args = parser.parse_args()

webapp_dir = args.outdir
freeciv_dir = args.freeciv

input_name = path.join(freeciv_dir, "data", "stdsounds.soundspec")

print("Parsing " + input_name)
config = configparser.ConfigParser()
config.read(input_name)

output_name = path.join(webapp_dir, 'javascript', 'soundset_spec.js')
with open(output_name, 'w') as f:
  f.write("var soundset = {");

  for key in config['files']:
    f.write("\"" + key + "\" : " );
    f.write(config['files'][key].replace("stdsounds/","").split(";")[0]);
    f.write(", ");

  f.write("\"last\":null};");

print("Generated " + output_name)
