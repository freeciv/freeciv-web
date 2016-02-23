#! /usr/bin/env python

'''**********************************************************************
    Copyright (C) 2009-2016  The Freeciv-web project

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
import datetime
import mysql.connector
import configparser
import http.client

settings = configparser.ConfigParser()
settings.read("../../pbem/settings.ini")

mysql_user=settings.get("Config", "mysql_user")
mysql_database=settings.get("Config", "mysql_database");
mysql_password=settings.get("Config", "mysql_password");

server_map = {};

def increment_metaserver_stats():
  result = None;
  cursor = None;
  cnx = None;
  try:
    cnx = mysql.connector.connect(user=mysql_user, database=mysql_database, password=mysql_password)
    cursor = cnx.cursor()
    query = ("INSERT INTO games_played_stats (statsDate, gameType, gameCount) VALUES (CURDATE(), 3, 1)  ON DUPLICATE KEY UPDATE gameCount = gameCount + 1;")
    cursor.execute(query);
    cnx.commit();
  finally:
    cursor.close()
    cnx.close()

def poll_metaserver():
  print("polling metaserver. server list size=" + str(len(server_map.keys())));
  conn = http.client.HTTPConnection("meta.freeciv.org")
  conn.request("GET", "/");
  r1 = conn.getresponse();
  html_doc = r1.read();
  all_rows = html_doc.decode('ascii').split("<tr>");
  rows = all_rows[2:len(all_rows)-1];
  for row in rows:
    cells = row.split("<");
    hostname_port = cells[2];
    state = cells[11];
    if (hostname_port in server_map):
      old_state = server_map[hostname_port];
      if ("Pregame" in old_state and "Running" in state):
        print("new game started for: " + hostname_port);
        increment_metaserver_stats();
    server_map[hostname_port] = state;

if __name__ == '__main__':
  print("Freeciv-web meta.freeciv.org stats");

  while (1):
    try:
      time.sleep(1);
      poll_metaserver();
      time.sleep(120);
    except Exception as e:
      print(e);
