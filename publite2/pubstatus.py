'''**********************************************************************
 Publite2 is a process manager which launches multiple Freeciv-web servers.
    Copyright (C) 2009-2019  The Freeciv-web project

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
from tornado import web, ioloop, httpserver
from processfinder import *

STATUS_PORT = 4002

"""Serves the Publite2 status page, on the url:  /pubstatus """
class PubStatus(Thread):
  def __init__(self, mc):
    Thread.__init__(self)
    self.metachecker = mc

  def run(self):
    io_loop = ioloop.IOLoop()
    io_loop.make_current()
    application = web.Application([
            (r"/pubstatus", StatusHandler, dict(metachecker=self.metachecker)),
    ])
    http_server = httpserver.HTTPServer(application)
    http_server.listen(STATUS_PORT)
    io_loop.start()

class StatusHandler(web.RequestHandler):
  def initialize(self, metachecker):
    self.metachecker = metachecker

  def get(self):
    game_count = 0;
    for server in self.metachecker.server_list:
      game_count += server.num_start;
    error_count = 0;
    for server in self.metachecker.server_list:
      error_count += server.num_error;

    error_rate = 0;
    if (game_count > 0): error_rate = (error_count * 100) / game_count;
      
    self.write("<html><head><title>Freeciv-web Admin</title>" + 
               "<link href='/css/bootstrap.min.css' rel='stylesheet'>" +
               "<meta http-equiv=\"refresh\" content=\"20\"><style>td { padding: 2px;}</style></head><body>");
    self.write("<div class='container'><div class='row'><div class='col-md-6'>"+
               "<h2>Freeciv-web Admin</h2>" + 
               "<table class='table table-bordred table-striped' style='font-size:80%;'>"+
               "<tr><td>Number of Freeciv-web games run:</td><td>" + str(game_count) + "</td></tr>" +
               "<tr><td>Server limit (maximum number of running servers):</td><td>" + str(self.metachecker.server_limit) + "</td></tr>" +
               "<tr><td>Server capacity:</td><td>" + str(self.metachecker.server_capacity_single) + "," +  
               str(self.metachecker.server_capacity_multi) + "," +  str(self.metachecker.server_capacity_pbem) + "</td></tr>" +
               "<tr><td>Number of servers running according to Publite2:</td><td>" + str(len(self.metachecker.server_list)) + "</td></tr>"
               "<tr><td>Number of servers running according to metaserver:</td><td>" + str(self.metachecker.total) + "</td></tr>" +
               "<tr><td>Available single-player pregame servers on metaserver:</td><td>" + str(self.metachecker.single) + "</td></tr>" +
               "<tr><td>Available multi-player pregame servers on metaserver:</td><td>" + str(self.metachecker.multi) + "</td></tr>" +
               "<tr><td>Available Play-by-email pregame servers on metaserver:</td><td>" + str(self.metachecker.pbem) + "</td></tr>" +
               "<tr><td>Number of HTTP checks against metaserver: </td><td>" + str(self.metachecker.check_count) + "</td></tr>" +
               "<tr><td>Last response from metaserver: </td><td>" + str(self.metachecker.html_doc) + "</td></tr>" +
               "<tr><td>Last HTTP status from metaserver: </td><td>" + str(self.metachecker.last_http_status) + "</td></tr>" +
               "<tr><td>Number of Freeciv servers stopped by error:</td><td>" + str(error_count) + "  " +
               "(" + str("{0:.2f}".format(error_rate)) + "%)</td></tr>" +
               "<tr><td>Pbem status:</td><td><a href='/pbemstatus'>/pbemstatus</a></td></tr>" +
               "<tr><td>Mail status (json):</td><td><a href='/mailstatus'>/mailstatus</a></td></tr>" +
               "</table></div></div>")

    self.write("<div class='row'><div class='col-md-12'><h3>Running Freeciv-web servers:</h3>");
    self.write("<table class='table table-bordred table-striped'><tr><td>Server Port</td><td>Type</td><td>Started time</td><td>Restarts</td>"+
            "<td>Errors</td><td>Freeciv PIDs</td><td>Freeciv-proxy PIDs</td></tr>");
    for server in self.metachecker.server_list:
      proxy_port = server.new_port + 1000;  
      self.write("<tr><td><a href='/civsocket/" + str(proxy_port) 
                 + "/status'>" + str(server.new_port) + "</a></td>" + "<td>" + str(server.gametype) 
                 +  "</td><td>" + str(server.started_time) + "</td><td>" + str(server.num_start) + "</td><td>" + str(server.num_error) +  "</td>"
                 + "<td>" + get_pid_of_process("freeciv-web", str(server.new_port)) + "</td>"
                 + "<td>" + get_pid_of_process("python3", str(proxy_port)) + "</td></tr>");
    self.write("</table></div></div></div>");
    self.write("</body></html>");



