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
from password_utils import verify_password, hash_password, needs_upgrade
import json
import uuid
import gc
import MySQLdb
import configparser
import urllib.request
import urllib.parse

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
            # Called the first time the user connects.
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

        # Get the civcom instance which corresponds to this user.
        if (self.is_ready):
            self.civcom = self.get_civcom(self.username, self.civserverport, self)

        if (self.civcom is None):
            self.write_message("[{\"pid\":5,\"message\":\"Error: Could not authenticate user.\",\"you_can_join\":false,\"conn_id\":-1}]")
            return

        # Send JSON request to freeciv-server.
        self.civcom.queue_to_civserver(message)

    def on_close(self):
        if hasattr(self, 'civcom') and self.civcom is not None:
            self.civcom.stopped = True
            self.civcom.close_connection()
            if self.civcom.key in list(civcoms.keys()):
                del civcoms[self.civcom.key]
            del(self.civcom)
            gc.collect()

    def check_user(self, username, token):
      """Authenticate a user via password or Google sign-in.

      Delegates to the appropriate authentication method based on the
      game's configuration (password-based or Google OAuth).

      Args:
          username: The username to authenticate.
          token: The password hash or Google OAuth token.

      Returns:
          True if authentication succeeds, False otherwise.
      """
      cursor = None
      cnx = None
      try:
        cnx = MySQLdb.connect(user=mysql_user, password=mysql_password, database=mysql_database)
        cursor = cnx.cursor()

        auth_method = self.get_game_auth_method(cursor)
        if auth_method == "password":
          return self.check_user_password(cursor, cnx, username, token)
        elif auth_method == "google":
          return self.check_user_google(username, token)
        else:
          return False

      finally:
        if cursor is not None:
          cursor.close()
        if cnx is not None:
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

    def check_user_password(self, cursor, cnx, username, password):
        """Verify a user's password against the database.

        Supports both bcrypt and legacy SHA-256 hash formats. On successful
        authentication with a legacy SHA-256 hash, the stored hash is
        transparently upgraded to bcrypt.

        Args:
            cursor: Active database cursor.
            cnx: Database connection (required for committing hash upgrades).
            username: The username to authenticate.
            password: The password token to verify.

        Returns:
            True if authentication succeeds, False otherwise.
        """
        query = ("select secure_hashed_password, activated from auth "
                 "where lower(username)=lower(%(usr)s)")
        cursor.execute(query, {'usr': username})
        result = cursor.fetchall()

        if len(result) == 0:
            # Unreserved user, no password needed
            return True

        for stored_hash, active in result:
            if active == 0:
                return False
            if verify_password(password, stored_hash):
                # Transparently upgrade legacy SHA-256 hashes to bcrypt.
                if needs_upgrade(stored_hash):
                    self._upgrade_password_hash(cursor, cnx, username, password)
                return True

        return False

    def _upgrade_password_hash(self, cursor, cnx, username, password):
        """Upgrade a legacy password hash to bcrypt in the database.

        Called transparently after successful authentication with a legacy
        hash format. This is best-effort: failures are logged but do not
        affect the authentication result.

        Args:
            cursor: Active database cursor.
            cnx: Database connection for committing the update.
            username: The username whose hash is being upgraded.
            password: The verified password token to re-hash with bcrypt.
        """
        try:
            new_hash = hash_password(password)
            update_query = ("UPDATE auth SET secure_hashed_password = %(hash)s "
                           "WHERE lower(username) = lower(%(usr)s)")
            cursor.execute(update_query, {'hash': new_hash, 'usr': username})
            cnx.commit()
            logger.info("Upgraded password hash to bcrypt for user: %s", username)
        except Exception as e:
            logger.error("Failed to upgrade password hash for user %s: %s",
                        username, e)

    def check_user_google(self, username, token):
        # Check login with Google Account
        try:
            request = urllib.request.Request('http://localhost:8080/freeciv-web/token_signin', data=urllib.parse.urlencode({'idtoken': token, 'username': username}).encode('ascii'), headers={'X-Real-IP': 'proxy'})
            return urllib.request.urlopen(request).read().decode('ascii') == 'OK'
        except Exception as e:
            logger.warn(e)
            return False

    # Enables support for allowing alternate origins. See check_origin in websocket.py
    def check_origin(self, origin):
      return True;

    # This enables WebSocket compression with default options.
    def get_compression_options(self):
        return {'compression_level' : 9, 'mem_level' : 9}

    # Get the civcom instance which corresponds to the requested user.
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
