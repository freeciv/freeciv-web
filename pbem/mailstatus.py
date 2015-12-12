from threading import Thread
from tornado import web, websocket, ioloop, httpserver
from tornado.ioloop import IOLoop

STATUS_PORT = 4003

"""Serves the PBEM status page, on the url:  /mailstatus """
class MailStatus(Thread):
  def __init__(self):
    Thread.__init__(self)

  def run(self):
    application = web.Application([
            (r"/mailstatus", StatusHandler, dict(mailchecker=self)),
    ])

    http_server = httpserver.HTTPServer(application)
    http_server.listen(STATUS_PORT)
    ioloop.IOLoop.instance().start()

class StatusHandler(web.RequestHandler):
  def initialize(self, mailchecker):
    self.mailchecker = mailchecker

  def get(self):
      
    self.write("<html><head><title>Mail status for Freeciv-web</title>" + 
               "<link href='//play.freeciv.org/css/bootstrap.min.css' rel='stylesheet'>" +
               "<meta http-equiv=\"refresh\" content=\"20\"><style>td { padding: 2px;}</style></head><body>");
    self.write("<div class='container'><h2>Freeciv-web PBEM status</h2>" + 
               "<table><tr><td>Number of e-mails sent:</td><td>" + str(self.mailchecker.emails_sent) + "</td></tr>" +
               "<tr><td>Number of savegames read:</td><td>" + str(self.mailchecker.savegames_read) + "</td></tr>" +
               "<tr><td>Number of ranklog emails sent:</td><td>" + str(self.mailchecker.ranklog_emails_sent) + "</td></tr>" +
               "</table>")
    self.write("</body></html>");
