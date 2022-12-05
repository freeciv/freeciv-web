#!/usr/bin/env python3
# -*- coding: utf-8 -*-

'''**********************************************************************
    Freeciv-web - the web version of Freeciv. https://www.freeciv.org/
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


from os import chdir
import re
import sys
from tornado import web, websocket, ioloop, httpserver
from debugging import *
import logging
from civcom import *
import json
import uuid
import gc
import mysql.connector
import configparser
import urllib.request
import urllib.parse
import hashlib

PROXY_PORT = 8002
CONNECTION_LIMIT = 1000

civcoms = {}

chdir(sys.path[0])
settings = configparser.ConfigParser()
settings.read("settings.ini")

mysql_user = settings.get("Config", "mysql_user")
mysql_database = settings.get("Config", "mysql_database")
mysql_password = settings.get("Config", "mysql_password")

google_signin = settings.get("Config", "google_signin")


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
    io_loop = ioloop.IOLoop.current()

    def open(self):
        self.id = str(uuid.uuid4())
        self.is_ready = False
        self.set_nodelay(True)

    def on_message(self, message):
        if (not self.is_ready and len(civcoms) <= CONNECTION_LIMIT):
            # called the first time the user connects.
            login_message = json.loads(message)
            self.username = login_message['username']
            if (not validate_username(self.username)):
              logger.warn("invalid username: " + str(message))
              self.write_message("[{\"pid\":5,\"message\":\"Error: Could not authenticate user. If you find a bug, please report it.\",\"you_can_join\":false,\"conn_id\":-1}]")
              return
            self.civserverport = login_message['port']
            auth_ok = self.check_user(
                    login_message['username'] if 'username' in login_message else None,
                    login_message['password'] if 'password' in login_message else None)
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

    # Check user authentication
    def check_user(self, username, token):
      cursor = None
      cnx = None
      try:
        cnx = mysql.connector.connect(user=mysql_user, database=mysql_database, password=mysql_password)
        cursor = cnx.cursor()

        auth_method = self.get_game_auth_method(cursor)
        if auth_method == "password":
          return self.check_user_password(cursor, username, token)
        elif auth_method == "google":
          return self.check_user_google(username, token)
        else:
          return False

      finally:
        cursor.close()
        cnx.close()

    # Returns the auth method for this game
    # Right now this is:
    # - Google account for otpd if a client key is defined
    # - password for any other case
    def get_game_auth_method(self, cursor):
        if google_signin is None or len(google_signin.strip()) == 0:
            return "password"
        query = ("select count(*) from servers where port=%(port)s and type='longturn'")
        cursor.execute(query, {'port': self.civserverport})
        if cursor.fetchall()[0][0] > 0:
            return "google"
        else:
            return "password"

    def check_user_password(self, cursor, username, password):
        query = ("select secure_hashed_password, activated from auth where lower(username)=lower(%(usr)s)")
        cursor.execute(query, {'usr': username, 'pwd': password})
        result = cursor.fetchall()

        if len(result) == 0:
            # Unreserved user, no password needed
            return True

        for secure_shashed_password, active in result:
            if (active == 0): return False
            if secure_shashed_password == hashlib.sha256(password.encode('utf-8')).hexdigest(): return True

        return False

    def check_user_google(self, username, token):
        # Check login with Google Account
        try:
            request = urllib.request.Request('http://localhost:8080/freeciv-web/token_signin', data=urllib.parse.urlencode({'idtoken': token, 'username': username}).encode('ascii'), headers={'X-Real-IP': 'proxy'})
            return urllib.request.urlopen(request).read().decode('ascii') == 'OK'
        except Exception as e:
            logger.warn(e)
            return False

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


def validate_username(name):
    if (name is None or len(name) <= 2 or len(name) >= 32):
        return False
    name = name.lower()
    return name != "pbem" and re.fullmatch('[a-z][a-z0-9]*', name) is not None


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
        ioloop.IOLoop.current().start()

    except KeyboardInterrupt:
        print('Exiting...')
