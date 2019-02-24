#!/usr/bin/env python3

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

from tornado import web, ioloop, httpserver
import io
import time

STATUS_PORT = 3999
savedir = "/var/lib/tomcat8/webapps/data/savegames/";
savegame_filename = "freeciv-earth-savegame-";
savecounter = 10000;
savetemplate = "";
with open('template_map.sav', 'r') as myfile:
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
    ioloop.IOLoop.current().start()

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
    if len(msg_part) != 3:
      self.set_status(400, "Wrong number of parameters: " + str(len(msg_part)))
      return

    users_map_string = msg_part[0];
    try:
      map_xsize = int(msg_part[1]);
      map_ysize = int(msg_part[2]);
    except ValueError:
      self.set_status(400, "Incorrect map size ["
                           + msg_part[1][:10] + ", "
                           + msg_part[2][:10] + "]")
      return

    # validate users_map_string.
    if (len(users_map_string) > 1000000 or (map_xsize * map_ysize > 18000)
        or map_xsize < 10 or map_ysize < 10):
      self.set_status(400, "Incorrect map size ["
                           + msg_part[1][:10] + ", "
                           + msg_part[2][:10] + ", "
                           + str(len(users_map_string)) + "]")
      return

    line_n = 0
    for line in io.StringIO(users_map_string):
      if len(line) != map_xsize + 9:
        self.set_status(400, "Incorrect row length: "
                             + str(line_n) + ", "
                             + str(len(line)) + ", "
                             + str(map_xsize))
        return
      if ((not line.startswith('t' + ('0000'+str(line_n))[-4:] + '="'))
          or (not line.endswith('"\n'))):
        self.set_status(400, "Incorrect row format: "
                             + str(line_n) + ", "
                             + line)
        return
      for c in line[7:-2]:
        if c not in "i+ :adfghjmpst":
          self.set_status(400, "Incorrect terrain: "
                               + str(line_n) + ", "
                               + bytes(c, "utf-8").hex())
          return
      line_n += 1

    if line_n != map_ysize:
      self.set_status(400, "Incorrect number of rows: "
                           + str(line_n) + ", "
                           + str(map_ysize))
      return

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

