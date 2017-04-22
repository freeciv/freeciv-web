# -*- coding: utf-8 -*-

'''
 Freeciv - Copyright (C) 2009-2014 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
'''

import socket
import select
from struct import *
from threading import Thread
import logging
import time

HOST = '127.0.0.1'
logger = logging.getLogger("freeciv-proxy")

# The CivCom handles communication between freeciv-proxy and the Freeciv C
# server.


class CivCom(Thread):

    def __init__(self, username, civserverport, key, civwebserver):
        Thread.__init__(self)
        self.socket = None
        self.username = username
        self.civserverport = civserverport
        self.key = key
        self.send_buffer = []
        self.connect_time = time.time()
        self.civserver_messages = []
        self.stopped = False
        self.packet_size = -1
        self.net_buf = bytearray(0)
        self.header_buf = bytearray(0)
        self.daemon = True
        self.civwebserver = civwebserver

    def run(self):
        # setup connection to civserver
        if (logger.isEnabledFor(logging.INFO)):
            logger.info("Start connection to civserver for " + self.username
                        + " from IP " + self.civwebserver.ip)
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setblocking(True)
        self.socket.settimeout(2)
        try:
            self.socket.connect((HOST, self.civserverport))
            self.socket.settimeout(0.01)
        except socket.error as reason:
            self.send_error_to_client(
                "Proxy unable to connect to civserver. Error: %s" %
                (reason))
            return

        # send initial login packet to civserver
        self.civserver_messages = [self.civwebserver.loginpacket]
        self.send_packets_to_civserver()

        # receive packets from server
        while True:
            packet = self.read_from_connection()

            if (self.stopped):
                return

            if (packet is not None):
                self.net_buf += packet

                if (len(self.net_buf) == self.packet_size and self.net_buf[-1] == 0):
                    # valid packet received from freeciv server, send it to
                    # client.
                    self.send_buffer_append(self.net_buf[:-1])
                    self.packet_size = -1
                    self.net_buf = bytearray(0)
                    continue

            time.sleep(0.01)
            # prevent max CPU usage in case of error

    def read_from_connection(self):
        try:
            if (self.socket is not None and not self.stopped):
                if (self.packet_size == -1):
                    self.header_buf += self.socket.recv(2 -
                                                        len(self.header_buf))
                    if (len(self.header_buf) == 0):
                        self.close_connection()
                        return None
                    if (len(self.header_buf) == 2):
                        header_pck = unpack('>H', self.header_buf)
                        self.header_buf = bytearray(0)
                        self.packet_size = header_pck[0] - 2
                        if (self.packet_size <= 0 or self.packet_size > 32767):
                            logger.error("Invalid packet size " + str(self.packet_size))
                    else:
                        # complete header not read yet. return now, and read
                        # the rest next time.
                        return None

            if (self.socket is not None and self.net_buf is not None and self.packet_size > 0):
                data = self.socket.recv(self.packet_size - len(self.net_buf))
                if (len(data) == 0):
                    self.close_connection()
                    return None

                return data
        except socket.timeout:
            self.send_packets_to_client()
            self.send_packets_to_civserver()
            return None
        except OSError:
            return None

    def close_connection(self):
        if (logger.isEnabledFor(logging.INFO)):
            logger.info(
                "Server connection closed. Removing civcom thread for " +
                self.username)

        if (hasattr(self.civwebserver, "civcoms") and self.key in list(self.civwebserver.civcoms.keys())):
            del self.civwebserver.civcoms[self.key]

        if (self.socket is not None):
            self.socket.close()
            self.socket = None
        self.civwebserver = None
        self.stopped = True

    # queue messages to be sent to client.
    def send_buffer_append(self, data):
        try:
            self.send_buffer.append(
                data.decode(
                    encoding="utf-8",
                    errors="ignore"))
        except UnicodeDecodeError:
            if (logger.isEnabledFor(logging.ERROR)):
                logger.error(
                    "Unable to decode string from civcom socket, for user: " +
                    self.username)
            return

    # sends packets to client (WebSockets client / browser)
    def send_packets_to_client(self):
        packet = self.get_client_result_string()
        if (packet is not None and self.civwebserver is not None):
            # Calls the write_message callback on the next Tornado I/O loop iteration (thread safely).
            self.civwebserver.io_loop.add_callback(lambda: self.civwebserver.write_message(packet))

    def get_client_result_string(self):
        result = ""
        try:
            if len(self.send_buffer) > 0:
                result = "[" + ",".join(self.send_buffer) + "]"
            else:
                result = None
        finally:
            del self.send_buffer[:]
        return result

    def send_error_to_client(self, message):
        if (logger.isEnabledFor(logging.ERROR)):
            logger.error(message)
        self.send_buffer_append(
            ("{\"pid\":25,\"message\":\"" + message + "\"}").encode("utf-8"))

    # Send packets from freeciv-proxy to civserver
    def send_packets_to_civserver(self):
        if (self.civserver_messages is None or self.socket is None):
            return

        try:
            for net_message in self.civserver_messages:
                utf8_encoded = net_message.encode('utf-8')
                header = pack('>H', len(utf8_encoded) + 3)
                self.socket.sendall(
                    header +
                    utf8_encoded +
                    b'\0')
        except:
            self.send_error_to_client(
                "Proxy unable to communicate with civserver on port " + str(self.civserverport))
        finally:
            self.civserver_messages = []

    # queue message for the civserver
    def queue_to_civserver(self, message):
        self.civserver_messages.append(message)
