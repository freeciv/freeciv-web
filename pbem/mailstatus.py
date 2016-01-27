from threading import Thread
from tornado import web, websocket, ioloop, httpserver
from tornado.ioloop import IOLoop
import json
from mailsender import MailSender
import time

STATUS_PORT = 4003

"""Serves the PBEM status page, on the url:  /mailstatus """
class MailStatus(Thread):
  def __init__(self):
    Thread.__init__(self)
    self.last_request = time.time();
    self.last_invitation = None;

  def run(self):
    application = web.Application([
            (r"/mailstatus", StatusHandler, dict(mailchecker=self)),
    ])

    http_server = httpserver.HTTPServer(application)
    http_server.listen(STATUS_PORT)
    ioloop.IOLoop.instance().start()

class StatusHandler(web.RequestHandler):
  def initialize(self, mailchecker):
    self.mailchecker = mailchecker;

  def ratelimit(self):
   if (time.time() - self.mailchecker.last_request < 1):
      self.mailchecker.last_request = time.time();
      raise Exception("too frequent requests");
   self.mailchecker.last_request = time.time();


  def get(self):
    req_ip = self.request.headers.get("X-Real-IP", "missing");
    print("request from " + req_ip);
    self.ratelimit();
 
    self.write("<html><head><title>Mail status for Freeciv-web</title>" + 
               "<link href='//play.freeciv.org/css/bootstrap.min.css' rel='stylesheet'>" +
               "<meta http-equiv=\"refresh\" content=\"20\"><style>td { padding: 2px;}</style></head><body>");
    self.write("<div class='container'><h2>Freeciv-web PBEM status</h2>" + 
               "<table><tr><td>Number of e-mails sent:</td><td>" + str(self.mailchecker.emails_sent) + "</td></tr>" +
               "<tr><td>Number of savegames read:</td><td>" + str(self.mailchecker.savegames_read) + "</td></tr>" +
               "<tr><td>Number of ranklog emails sent:</td><td>" + str(self.mailchecker.ranklog_emails_sent) + "</td></tr>" +
               "<tr><td>Number of invitation emails sent:</td><td>" + str(self.mailchecker.invitation_emails_sent) + "</td></tr>" +
               "<tr><td>Status of games:</td><td>" + json.dumps(self.mailchecker.games) + "</td></tr>" +
               "</table>")
    self.write("</body></html>");

  def post(self):
    req_ip = self.request.headers.get("X-Real-IP", "missing");
    self.mailchecker.invitation_emails_sent += 1;
    invite_to = self.get_argument("to", None, True);

    action=self.get_argument("action", None, True);
    print("request from " + req_ip + " to " + invite_to);
    self.ratelimit();
    if (self.mailchecker.last_invitation == invite_to):
      self.set_status(502);
      print("error: invitation to same e-mail twice.");
      return;

    self.mailchecker.last_invitation = invite_to;
    if (action == "invite"):
      invite_from = self.get_argument("from", None, True)
      if (invite_from != None and invite_to != None and "@" in invite_to and len(invite_from) < 50 and len(invite_to) < 50):
        m = MailSender();
        m.send_invitation(invite_from, invite_to);
    self.write("done!");


