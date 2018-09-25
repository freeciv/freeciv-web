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

from threading import Thread
from tornado import web, websocket, ioloop, httpserver
from tornado.ioloop import IOLoop
from os import environ, path
import re
import time

def _get_relative_path(filepath):
  return path.join(path.dirname(__file__), filepath)

STATUS_PORT = 3999
savedir = environ['FREECIV_WEB_SAVE_GAME_DIRECTORY']
savegame_filename = "freeciv-earth-savegame-";
savecounter = 10000;
savetemplate = "";
with open(_get_relative_path('template_map.sav'), 'r') as myfile:
    savetemplate=myfile.read();
myfile.close();

"""  /freeciv-earth-mapgen """
class MapGen():

  def start(self):
    application = web.Application([
            (r"/freeciv-earth-mapgen", MapHandler),
    ])

    http_server = httpserver.HTTPServer(application)
    http_server.listen(STATUS_PORT)
    ioloop.IOLoop.instance().start()

class MapHandler(web.RequestHandler):

  def get(self):
    global savecounter;
    self.write("Freeciv-Earth map generator! (" + str(savecounter) + ")");

  def post(self):
    global savecounter;
    time.sleep(1);
    print("handling request " + str(savecounter) + " to generate savegame for Freeciv-Earth.");
    self.set_header("Content-Type", "text/plain")
    client_request = self.request.body.decode("utf-8");
    msg_part = client_request.split(";");
    users_map_string = msg_part[0];
    map_xsize = int(msg_part[1]);
    map_ysize = int(msg_part[2]);

    # validate users_map_string.
    if (len(users_map_string) > 1000000 or (map_xsize * map_ysize > 18000)):
      self.set_status(504)
      return;

    for c in users_map_string:
      if not (c=='a' or c=='g' or c=='t' or c=='d' or c=='f' or c=='p' \
         or c=='m' or c=='h' or c==':' or c==' ' or c=='\n' \
         or c=='"' or c=='=' or c=="0" or c=="1" or c=="2" or c=="3" \
         or c=="4" or c=="5" or c=="6" or c=="7" or c=="8" or c=="9"): 
        print("error:" + c);
        self.set_status(503)
        return;

    user_new_savegame = savetemplate.replace("{xsize}", str(map_xsize)).replace("{ysize}", str(map_ysize)) + users_map_string;

    # save result to data webapp.
    savecounter += 1;
    new_filename =  savegame_filename + str(savecounter); 
    save_file = open(savedir + new_filename + ".sav", "w");
    try:
      save_file.write(user_new_savegame);
    finally:
      save_file.close()

    #return new savegame filename to user.
    self.write(new_filename);

if __name__ == '__main__':
  print("Freeciv-Earth processing maps");
  mg = MapGen();
  mg.start();

