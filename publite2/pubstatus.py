from threading import Thread
from tornado import web, websocket, ioloop, httpserver
from tornado.ioloop import IOLoop

STATUS_PORT = 4002

"""Serves the Publite2 status page, on the url:  /pubstatus """
class PubStatus(Thread):
  def __init__(self, mc):
    Thread.__init__(self)
    self.metachecker = mc

  def run(self):
    application = web.Application([
            (r"/pubstatus", StatusHandler, dict(metachecker=self.metachecker)),
    ])

    http_server = httpserver.HTTPServer(application)
    http_server.listen(STATUS_PORT)
    ioloop.IOLoop.instance().start()

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
      
    self.write("<html><head><title>Publite2 status for Freeciv-web</title>" + 
               "<link href='//play.freeciv.org/css/bootstrap.min.css' rel='stylesheet'>" +
               "<meta http-equiv=\"refresh\" content=\"20\"><style>td { padding: 2px;}</style></head><body>");
    self.write("<div class='container'><h2>Freeciv-web Publite2 status</h2>" + 
               "<table><tr><td>Number of Freeciv-web games run:</td><td>" + str(game_count) + "</td></tr>" +
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
               "<tr><td>Mail status:</td><td><a href='/mailstatus'>/mailstatus</a></td></tr>" +
               "</table>")
    self.write("<h3>Running Freeciv-web servers:</h3>");
    self.write("<table><tr><td>Server Port</td><td>Type</td><td>Started time</td><td>Restarts</td><td>Errors</td></tr>");
    for server in self.metachecker.server_list:
      self.write("<tr><td><a href='/civsocket/" + str(server.new_port + 1000) 
                 + "/status'>" + str(server.new_port) + "</a></td>" + "<td>" + str(server.gametype) 
                 +  "</td><td>" + str(server.started_time) + "</td><td>" + str(server.num_start) + "</td><td>" + str(server.num_error) +  "</td></tr>");
    self.write("</table></div>");
    self.write("</body></html>");
