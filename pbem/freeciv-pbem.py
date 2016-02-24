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


import os, os.path
import sys
import time
import lzma
import datetime
import mysql.connector
from mailsender import MailSender
from mailstatus import *
import shutil
import random
import configparser
import json

savedir = "../resin/webapps/data/savegames/" 
rankdir = "../resin/webapps/data/ranklogs/" 
game_expire_time = 60 * 60 * 24 * 7;  # 7 days until games are expired.

settings = configparser.ConfigParser()
settings.read("settings.ini")

mysql_user=settings.get("Config", "mysql_user")
mysql_database=settings.get("Config", "mysql_database");
mysql_password=settings.get("Config", "mysql_password");

# load game status from file.
loaded_games = {};
try:
  if (os.path.isfile('pbem-games.json')):
    with open('pbem-games.json') as data_file:    
      loaded_games = json.load(data_file)
except Exception as e:
  print(e);

status = MailStatus()
status.savegames_read = 0;
status.emails_sent = 0;
status.ranklog_emails_sent = 0;
status.invitation_emails_sent = 0;
status.retired = 0;
status.games = loaded_games;
status.start();

# remove old games 
def cleanup_stats():
  for key, value in status.games.copy().items():
    if ((value[4] + game_expire_time)< (time.time())):
      print("Expiring game: " + key);
      del status.games[key];
      status.retired += 1;

# parse Freeciv savegame and collect information to include in e-mail.
def handle_savegame(root, file):
  time.sleep(1);
  filename = os.path.join(root,file)
  print("Handling savegame: " + filename);
  txt = None;
  with lzma.open(filename,  mode="rt") as f:
    txt = f.read().split("\n");
    status.savegames_read += 1;

  new_filename = "pbem_processed_" + str(random.randint(0,10000000000)) + ".xz";
  f.close();
  shutil.move(filename, os.path.join(root,new_filename))
  print("New filename will be: " + new_filename);
  players = list_players(txt);
  phase = find_phase(txt);
  turn = find_turn(txt);
  game_id = find_game_id(txt);
  state = find_state(txt);
  print("game_id=" + str(game_id));
  print("phase=" + str(phase));
  print("turn=" + str(turn));
  print("state=" + str(state));
  print("players=" + str(players));

  active_player = players[phase];
  print("active_player=" + active_player);    
  active_email = find_email_address(active_player);
  status.games[game_id] = [turn, phase, players, time.ctime(), int(time.time()), state];
  if (active_email != None):
    print("active email=" + active_email);
    m = MailSender();
    m.send_email(active_player, players, active_email, new_filename.replace(".xz", ""), turn);
    status.emails_sent += 1;

  #store games status in file
  with open('pbem-games.json', 'w') as outfile:
    json.dump(status.games, outfile);

#Returns the phase (active player number), eg 1
def find_phase(lines):
  for l in lines:
    if ("phase=" in l): return int(l[6:]);
  return None;

#Returns the current turn
def find_turn(lines):
  for l in lines:
    if ("turn=" == l[0:5]): return int(l[5:]);
  return None;

#Returns the current server state
def find_state(lines):
  for l in lines:
    if ("server_state=" == l[0:13]): return l[18:].replace("\"","");
  return None;


#Returns the game id
def find_game_id(lines):
  for l in lines:
    if ("id=" == l[0:3]): return l[4:];
  return None;


# Returns a list of active players
def list_players(lines):
  players = [];
  for l in lines:
    if (l[:4] == "name"): players.append(l[5:].replace("\"", ""));
  return players;

# Returns the emailaddress of the given username
def find_email_address(user_to_find):
  result = None;
  cursor = None;
  cnx = None;
  try:
    cnx = mysql.connector.connect(user=mysql_user, database=mysql_database, password=mysql_password)
    cursor = cnx.cursor()
    query = ("select email from auth where lower(username)=lower(%(usr)s) and activated='1' limit 1")
    cursor.execute(query, {'usr' : user_to_find})
    for email in cursor:
      result = email[0];
  finally:
    cursor.close()
    cnx.close()
  return result;


def process_savegames():
  for root, subFolders, files in os.walk(savedir):
    for file in files:
      if (file.endswith(".xz") and file.startswith("pbem") and not file.startswith("pbem_processed")):
        handle_savegame(root, file);
        

def handle_ranklog(root, file):
  filename = os.path.join(root,file)
  print("Handling ranklog: " + filename);
  openfile = open(filename, 'r')
  lines = openfile.readlines()
  winner = None;
  winner_score = None;
  winner_email = None;
  losers = None;
  losers_score = None;
  losers_email = None;  

  turns = None;
  for line in lines:
    if (len(line) > 9 and line[:7]=='winners' and ',' in line):
      winner = line[9:].split(",")[1];  #FIXME: this is not always correct, if there are multiple winners.
      winner_score = line[9:].split(",")[3];
      winner_email = find_email_address(winner);
    if (len(line) > 8 and line[:6]=='losers' and ',' in line):
      losers = line[8:].split(",")[1];
      losers_score = line[8:].split(",")[3];
      losers_email = find_email_address(losers);
  if (losers_email != None and winner_email != None):
    m = MailSender();
    m.send_game_result_mail(winner, winner_score, winner_email, losers, losers_score, losers_email);
    status.ranklog_emails_sent += 1;
  else:
    print("error: game with winner without email in " + file);
  openfile.close();
  os.remove(filename);
  

def process_ranklogs():
  for root, subFolders, files in os.walk(rankdir):
    for file in files:
      if (file.endswith(".log")):
        handle_ranklog(root, file);
        
if __name__ == '__main__':
  print("Freeciv-PBEM processing savegames");

  while (1):
    try:
      time.sleep(5);
      process_savegames();
      process_ranklogs();
      cleanup_stats();
      time.sleep(60);
    except Exception as e:
      print(e);
