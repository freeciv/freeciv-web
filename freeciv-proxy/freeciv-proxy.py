#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
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


from os import path as op
from os import chdir
import sys
import time
from tornado import web, websocket, ioloop, httpserver
from tornado.ioloop import IOLoop
from debugging import *
import logging
from civcom import *
import json
import uuid
import gc
import mysql.connector
import configparser

PROXY_PORT = 8002
CONNECTION_LIMIT = 1000

civcoms = {}

chdir(sys.path[0])
settings = configparser.ConfigParser()
settings.read("settings.ini")

mysql_user=settings.get("Config", "mysql_user");
mysql_database=settings.get("Config", "mysql_database");
mysql_password=settings.get("Config", "mysql_password");

class IndexHandler(web.RequestHandler):

    """Serves the Freeciv-proxy index page """

    def get(self):
        self.write("Freeciv-web websocket proxy, port: " + str(PROXY_PORT))


class StatusHandler(web.RequestHandler):

    """Serves the Freeciv-proxy status page, on the url:  /status """

    def get(self, params):
        self.write(get_debug_info(civcoms))


class WSHandler(websocket.WebSocketHandler):
    logger = logging.getLogger("freeciv-proxy")
    io_loop = IOLoop.instance()

    def open(self):
        self.id = str(uuid.uuid4())
        self.is_ready = False
        self.set_nodelay(True)

    def on_message(self, message):
        if (not self.is_ready and len(civcoms) <= CONNECTION_LIMIT):
            # called the first time the user connects.
            login_message = json.loads(message)
            self.username = login_message['username']
            self.civserverport = login_message['port']
            auth_ok = self.check_user(
                    login_message['username'] if 'username' in login_message else None, 
                    login_message['password'] if 'password' in login_message else None, 
                    login_message['subject'] if 'subject' in login_message else None);
            if (not auth_ok): 
              self.write_message("[{\"pid\":5,\"message\":\"Error: Could not authenticate user with password. Try a different username.\",\"you_can_join\":false,\"conn_id\":-1}]")
              return

            self.loginpacket = message
            self.is_ready = True
            self.civcom = self.get_civcom(
                self.username,
                self.civserverport,
                self)
            return

        # get the civcom instance which corresponds to this user.
        if (self.is_ready): 
            self.civcom = self.get_civcom(self.username, self.civserverport, self)

        if (self.civcom is None):
            self.write_message("[{\"pid\":5,\"message\":\"Error: Could not authenticate user.\",\"you_can_join\":false,\"conn_id\":-1}]")
            return

        # send JSON request to civserver.
        self.civcom.queue_to_civserver(message)

    def on_close(self):
        if hasattr(self, 'civcom') and self.civcom is not None:
            self.civcom.stopped = True
            self.civcom.close_connection()
            if self.civcom.key in list(civcoms.keys()):
                del civcoms[self.civcom.key]
            del(self.civcom)
            gc.collect()

    # Check if username and password if correct, if the user already exists in the database.
    def check_user(self, username, password, check_subject):
      result = None;
      cursor = None;
      cnx = None;
      try:
        cnx = mysql.connector.connect(user=mysql_user, database=mysql_database, password=mysql_password)

        # Check login with Google Account
        cursor = cnx.cursor()
        query = ("select subject, activated, (select a.username from auth a where ga.username = a.username) as auth_username from google_auth ga where lower(username)=lower(%(usr)s)")
        cursor.execute(query, {'usr' : username})
        dbSubject = None;
        authUsername = None;
        for usrrow in cursor:
          if (usrrow[1] == 0): return False;
          dbSubject = usrrow[0];
          authUsername = usrrow[2];
        if (dbSubject is not None and dbSubject != check_subject and authUsername is None): return False;
        if (dbSubject is not None and dbSubject == check_subject): return True;

        # Get the hashed password from the database
        query = ("select username, secure_hashed_password, activated from auth where lower(username)=lower(%(usr)s)")
        cursor.execute(query, {'usr' : username})
        salt = None;
        for usrrow in cursor:
          salt = usrrow[1];
          if (usrrow[2] == 0): return False;
        if (salt == None): return True;

        # Validate the password in the database
        query = ("select count(*) from auth where lower(username)=lower(%(usr)s) and secure_hashed_password = ENCRYPT(%(pwd)s, %(salt)s)")
        cursor.execute(query, {'usr' : username, 'pwd' : password, 'salt' : salt})
        usrcount = 0;
        for authrow in cursor:
          usrcount = authrow[0];
        if (usrcount != 1): return False; 
      finally:
        cursor.close()
        cnx.close()
      return True;


    # enables support for allowing alternate origins. See check_origin in websocket.py
    def check_origin(self, origin):
      return True;

    # this enables WebSocket compression with default options.
    def get_compression_options(self):
        return {'compression_level' : 9, 'mem_level' : 9}

    # get the civcom instance which corresponds to the requested user.
    def get_civcom(self, username, civserverport, ws_connection):
        key = username + str(civserverport) + ws_connection.id
        if key not in list(civcoms.keys()):
            if (int(civserverport) < 5000):
                return None
            civcom = CivCom(username, int(civserverport), key, self)
            civcom.start()
            civcoms[key] = civcom

            return civcom
        else:
            return civcoms[key]


if __name__ == "__main__":
    try:
        print('Started Freeciv-proxy. Use Control-C to exit')

        if len(sys.argv) == 2:
            PROXY_PORT = int(sys.argv[1])
        print(('port: ' + str(PROXY_PORT)))

        LOG_FILENAME = '../logs/freeciv-proxy-logging-' + str(PROXY_PORT) + '.log'
        logging.basicConfig(filename=LOG_FILENAME,level=logging.INFO)
        logger = logging.getLogger("freeciv-proxy")

        application = web.Application([
            (r'/civsocket/' + str(PROXY_PORT), WSHandler),
            (r"/", IndexHandler),
            (r"(.*)status", StatusHandler),
        ])

        http_server = httpserver.HTTPServer(application)
        http_server.listen(PROXY_PORT)
        ioloop.IOLoop.instance().start()

    except KeyboardInterrupt:
        print('Exiting...')
