#! /usr/bin/env python

'''**********************************************************************
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


import os
import sys
import time
import lzma

rootdir = "/vagrant/resin/webapps/ROOT/savegames/" 

def handle_savegame(filename):
  print("Handling " + filename);
  with lzma.open(filename) as f:
    txt =str(f.read());
    phase = find_phase(txt);
    print(phase);
    print(txt);

def find_phase(lines):
  for l in lines:
    if ("phase=" in str(l)): return l;
  return None;
 
def process_savegames():
  for root, subFolders, files in os.walk(rootdir):
    for file in files:
      if (file.endswith(".xz")):
        f = os.path.join(root,file)
        handle_savegame(f);
        
        
if __name__ == '__main__':
  print("Freeciv-PBEM processing savegames");
  process_savegames();

  # TODO:
  # The idea of PDEM is that this script handles play-by-emailgames like this:
  # 1. parse freeciv savegames starting with the filename pbem*
  # 2. based on each savegame, determine which player name is next.
  # 3. look up the email-address of that player in the auth database table
  # 4. send the email to that player informing them that it is their turn.
  # 5. optionally, send an email to all players that the game is over.

  # TODO 2:
  # 1. add activation of email addreses (each new player will receive an activation-email with a 
  # link to activate their account. this will prevent spam.
  # 2. unsubscribe.


