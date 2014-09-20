from threading import Thread
from tornado import web, websocket, ioloop, httpserver
from tornado.ioloop import IOLoop
import sys
import traceback

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
    self.write("<html><body>");
    self.write("<h2>Freeciv-web Publite2 status</h2>" + 
               "<table><tr><td>Server limit (maximum number of running servers):</td><td>" + str(self.metachecker.server_limit) + "</td></tr>" +
               "<tr><td>Server capacity (number of servers in pregame):</td><td>" + str(self.metachecker.server_capacity) + "</td></tr>" +
               "<tr><td>Number of servers running according to Publite2:</td><td>" + str(len(self.metachecker.server_list)) + "</td></tr>"
               "<tr><td>Number of servers running according to metaserver:</td><td>" + str(self.metachecker.total) + "</td></tr>" +
               "<tr><td>Available single-player pregame servers on metaserver:</td><td>" + str(self.metachecker.single) + "</td></tr>" +
               "<tr><td>Available multi-player pregame servers on metaserver:</td><td>" + str(self.metachecker.multi) + "</td></tr>" +
               "<tr><td>Number of HTTP checks against metaserver: </td><td>" + str(self.metachecker.check_count) + "</td></tr>" +
               "<tr><td>Last response from metaserver: </td><td>" + str(self.metachecker.html_doc) + "</td></tr>" +
               "<tr><td>Last HTTP status from metaserver: </td><td>" + str(self.metachecker.last_http_status) + "</td></tr>" +
               "</table>")
    self.write("<h3>Running Freeciv-web servers:</h3>");
    self.write("<table><tr><td>Server Port</td><td>Proxy status</td><td>Type</td><td>Started time</td></tr>");
    for server in self.metachecker.server_list:
      self.write("<tr><td>" + str(server.new_port) + "</td><td><a href='/civsocket/" + str(server.new_port + 1000) 
                 + "/status'>status</a></td>" + "<td>" + str(server.gametype) 
                 +  "</td><td>" + str(server.started_time) + "</td></tr>");
    self.write("</table>");


    code = "<h3>Thread dumps:</h3>"
    for threadId, stack in list(sys._current_frames().items()):
      code += ("<br><b><u># ThreadID: %s</u></b><br>" % threadId)
      for filename, lineno, name, line in traceback.extract_stack(stack):
        code += ('File: "%s", line %d, in %s: ' % (filename, lineno, name))
        if line:
          code += (" <b>%s</b> <br>" % (line.strip()))
    self.write(code);
    self.write("</body></html>");
