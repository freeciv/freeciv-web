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
import smtplib
import json
import configparser
import re
import time
from os import environ, path

from email.mime.text import MIMEText

def _get_relative_path(filepath):
  return path.join(path.dirname(__file__), filepath)

class MailSender():
  testmode = False;

  smtp_login=environ['FREECIV_WEB_MAILGUN_EMAIL_USERNAME']
  smtp_password=environ['FREECIV_WEB_MAILGUN_EMAIL_PASSWORD']
  smtp_host='smtp.mailgun.org'
  smtp_port=587
  smtp_sender='Freeciv-web <postmaster@freecivweb.info>'

  host = environ['FREECIV_WEB_ROOT_URL']

  template_next_turn = "";
  with open(_get_relative_path('email_template_next_turn.html'), 'r') as content_file:
    template_next_turn = content_file.read();
  template_invitation = "";
  with open(_get_relative_path('email_template_invitation.html'), 'r') as content_file:
    template_invitation = content_file.read();
  template_generic = "";
  with open(_get_relative_path('email_template_generic.html'), 'r') as content_file:
    template_generic = content_file.read();


  def send_message_via_smtp(self, from_, to, mime_string):
    time.sleep(2);
    if not self.testmode:
      smtp = smtplib.SMTP(self.smtp_host, int(self.smtp_port))
      smtp.login(self.smtp_login, self.smtp_password);
      smtp.sendmail(from_, to, mime_string)
      smtp.quit()
    else:
      print("Test: this message would be sent: " + mime_string);


  def send_mailgun_message(self, to, subject, text):
    msg = MIMEText(text, _subtype='html', _charset='utf-8', )
    msg['Subject'] = subject
    msg['From'] = self.smtp_sender; 
    msg['To'] = to
    self.send_message_via_smtp(self.smtp_sender, to, msg.as_string())


  #Send the actual PBEM e-mail next turn invitation message.
  def send_email_next_turn(self, player_name, players, email_address, game_url, turn):
    print("Sending e-mail to: " + email_address);
    msg = self.template_next_turn;
    msg = msg.replace("{player_name}", player_name);
    msg = msg.replace("{game_url}", game_url);
    msg = msg.replace("{turn}", str(turn));
    plrs_txt = "";
    for p in players:
       plrs_txt += p + ", ";
    msg = msg.replace("{players}", plrs_txt);
    msg = msg.replace("{host}", self.host);

    self.send_mailgun_message(email_address, "Freeciv-Web: It's your turn to play! Turn: " \
        + str(turn), msg);

  def send_invitation(self, invitation_from, invitation_to):
    sender = re.sub(r'\W+', '', invitation_from);
    msg = self.template_invitation;
    msg = msg.replace("{sender}", sender);
    msg = msg.replace("{host}", self.host);

    self.send_mailgun_message(invitation_to, "Freeciv-Web: Join my game!" , msg);


  # send email with ranking after game is over.
  def send_game_result_mail(self, winner, winner_score, winner_email, losers, losers_score, losers_email):
    print("Sending ranklog result to " + winner_email + " and " + losers_email);
    msg_winner = "To " + winner + "<br>You have won the game of Freeciv-web against " + losers + ".<br>";
    msg_winner += "Winner score: " + winner_score + "<br>"
    msg_winner += "Loser score: " + losers_score + "<br>"
    msg = self.template_generic.replace("{message}", msg_winner);
    msg = msg.replace("{host}", self.host);
    self.send_mailgun_message(winner_email, "Freeciv-Web: You won!", msg);

    msg_loser = "To " + losers + "<br>You have lost the game of Freeciv-web against " + winner + ".<br>";
    msg_loser += "Winner score: " + winner_score + "<br>"
    msg_loser += "Loser score: " + losers_score + "<br>"
    msg = self.template_generic.replace("{message}", msg_loser);
    msg = msg.replace("{host}", self.host);
    self.send_mailgun_message(losers_email, "Freeciv-Web: You lost!", msg);

  # send email with reminder that game is about to expire
  def send_game_reminder(self, email, game_url):
    print("Sending game reminder email to " + email);
    msg = "Hello! This is a reminder that it is your turn to play Freeciv-web!<br><br>";
    msg += "<a href='" + game_url + "'>Click here to play your turn now</a> <br><br>";
    msg += "Your game will expire in 24 hours.<br>"
    msg = self.template_generic.replace("{message}", msg);
    msg = msg.replace("{host}", self.host);
    self.send_mailgun_message(email, "Freeciv-Web: Remember to play your turn!", msg);

