# -*- coding: latin-1 -*-

''' 
 Freeciv - Copyright (C) 2009-2013 - Andreas Røsdal   andrearo@pvv.ntnu.no
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
from threading import Thread;
import logging
import time


HOST = '127.0.0.1';
VERSION = "+Freeciv.Web.Devel-2.5-2013.May.02";  # Must be kept in sync with Freeciv server.
VER_INFO = "-dev";
logger = logging.getLogger("freeciv-proxy");

class CivCom(Thread):

  def __init__ (self, username, civserverport, civwebserver):
    Thread.__init__(self)
    self.socket = None;
    self.username = username;
    self.civserverport = civserverport;
    self.key = username + str(civserverport);
    self.send_buffer = [];
    self.connect_time = time.time();
    self.stopped = False;
    self.packet_size = -1;
    self.net_buf = bytearray(0);
    self.daemon = True;
    self.civwebserver = civwebserver;

  def run(self):
    #setup connection to civserver
    if (logger.isEnabledFor(logging.INFO)):
      logger.info("Start connection to civserver for " + self.username 
                  + " from IP " + self.civwebserver.ip)
    self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.socket.setblocking(True);
    self.socket.settimeout(2);
    try:
        self.socket.connect((HOST, self.civserverport))
        self.socket.settimeout(0.03);
    except socket.error as reason:
      self.send_error_to_client("Proxy unable to connect to civserver. Error: %s" % (reason));
      return;

    # send initial login packet to civserver
    self.send_to_civserver(self.civwebserver.loginpacket);

    #receive packets from server
    while 1:
      packet = self.read_from_connection(); 
          
      if (self.stopped):
        return;
              
      if (packet != None): 
        self.net_buf += packet;
        current_netbuf_len = len(self.net_buf);
        if (current_netbuf_len >= self.packet_size - 1):
          # valid packet received from freeciv server, send it to client.
          self.send_buffer_append(self.net_buf[:-1]);
          self.packet_size = -1;
          self.net_buf = bytearray(0);
          continue;
      time.sleep(0.01); #prevent max CPU usage in case of error

  def read_from_connection(self):
    try:
      if (self.socket != None and not self.stopped):    
        if (self.packet_size == -1):
          header_msg = self.socket.recv(4);
          if (header_msg == None): 
            self.close_connection();
            return None;
          if (len(header_msg) == 4):
            header_pck = unpack('>HH', header_msg[:4]);
            self.packet_size = header_pck[0] - 4;

      if (self.socket != None and self.net_buf != None and self.packet_size > 0):
        data = self.socket.recv(self.packet_size - len(self.net_buf));
        if (len(data) == 0):
          self.close_connection();
          return None;

        return data;
    except socket.timeout: 
      self.send_packets_to_client();
      return None;
    except OSError:
      return None;

  
  def close_connection(self):
    if (logger.isEnabledFor(logging.ERROR)):
      logger.error("Server connection closed. Removing civcom thread for " + self.username);
    
    if (hasattr(self.civwebserver, "civcoms") and self.key in list(self.civwebserver.civcoms.keys())):
      del self.civwebserver.civcoms[self.key];
    
    if (self.socket != None):
      self.socket.close();
      self.socket = None; 

    self.stopped = True;
    

  def send_to_civserver(self, net_packet_json):
    header = pack('>HH', len(net_packet_json), 0);
    try:
      # Send packet to civserver
      if self.socket != None:
        self.socket.sendall(header + str(net_packet_json).encode('utf-8') + b'\0');
      return True;
    except:
      self.send_error_to_client("Proxy unable to communicate with civserver.");
      return False;

  def send_buffer_append(self, data):
    self.send_buffer.append(data.decode("utf-8"));

  def send_packets_to_client(self):
    packet = self.get_send_result_string();
    if (packet != None and self.civwebserver != None):
      self.civwebserver.write_message(packet);


  def get_send_result_string(self):
    result = "";
    try:
      if len(self.send_buffer) > 0:
        result = "[" + ",".join(self.send_buffer) + "]";
      else:
        result = None;
    finally:
      del self.send_buffer[:];
    return result;

  def send_error_to_client(self, message):
    if (logger.isEnabledFor(logging.ERROR)):
      logger.error(message);
    self.send_buffer_append(("{\"pid\":18,\"message\":\"" + message + "\"}").encode("utf-8"));
     
