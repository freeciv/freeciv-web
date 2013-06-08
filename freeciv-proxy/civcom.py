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
from struct import *
from threading import Thread, RLock;
import logging
import time

HOST = 'localhost';
VERSION = "+Freeciv.Web.Devel-2.5-2013.May.02";  # Must be kept in sync with Freeciv server.
VER_INFO = "-dev";
logger = logging.getLogger("freeciv-proxy");

class CivCom(Thread):

  def __init__ (self, username, civserverport):
    Thread.__init__(self)
    self.socket = None;
    self.username = username;
    self.civserverport = civserverport;
    self.key = username + str(civserverport);
    self.send_buffer = [];
    self.lock = RLock();
    self.pingstamp = time.time();
    self.stopped = False;
    self.net_buf = bytearray(0);
    self.packet_len = -1;

    self.daemon = True;

  def set_civwebserver(self, civwebserver):
    self.civwebserver = civwebserver;


  def run(self):
    #setup connection to civserver
    if (logger.isEnabledFor(logging.INFO)):
      logger.info("Start connection to civserver.")
    self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.socket.settimeout(1); 
    self.socket.setblocking(1);
    try:
      self.socket.connect((HOST, self.civserverport))
    except socket.error as reason:
      self.send_error_to_client("Proxy unable to connect to civserver. Please go back and try again. %s" % (reason));
      return;


    # send packet
    self.send_to_civserver(self.civwebserver.loginpacket);

    #receive packets from server
    while 1:
      self.net_buf = self.read_from_connection();
    
      if (self.net_buf == None or self.stopped or self.socket == None):
        return;
              
      if (len(self.net_buf) == self.packet_len): 
        # valid packet received
        self.send_buffer_append(self.net_buf[:-1]);
        self.net_buf = bytearray(0);
        self.packet_len = -1;


  def read_from_connection(self):
    try:
      if (self.socket != None and not self.stopped):    
        if (self.packet_len == -1):
          header_msg = self.socket.recv(4);
          if not header_msg: 
            self.close_connection();
            return None;

          if (len(header_msg) == 4):
            header_pck = unpack('>HH', header_msg[:4]);
            self.packet_len = header_pck[0] - 4;

        if (self.socket is not None and self.packet_len >= 0 and self.net_buf != None):
          data = self.socket.recv(self.packet_len - len(self.net_buf));
          if not data: 
            self.close_connection();
            return None;

          if (len(data) == 0): 
            return self.net_buf;
          else:
            return self.net_buf + data;
      else:
        return None;

    except socket.error:
      if (logger.isEnabledFor(logging.ERROR)):
        logger.error("Server connection closed. Removing civcom thread for " + self.username);
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
    

  def send_packets_to_civserver(self, packet):
    if not self.send_to_civserver(packet): return False;
    return True;

  def send_to_civserver(self, net_packet_json):
    header = pack('>HH', len(net_packet_json), 0);
    try:
      # Send packet to civserver
      self.socket.sendall(header + str(net_packet_json).encode('utf-8') + b'\0');
      return True;
    except:
      self.send_error_to_client("Proxy unable to communicate with civserver.");
      self.civwebserver.close(); 
      return False;
    else:
      if (logger.isEnabledFor(logging.ERROR)):
        logger.error("invalid packet from 'json_to_civserver'");
      return False;

  def send_buffer_append(self, data):
    if (data == None or len(data) == 0): return;
    if not self.lock.acquire(False):
      if (logger.isEnabledFor(logging.DEBUG)):
        logger.debug("Could not acquire civcom lock");
    else:
      try:
        self.send_buffer.append(data.decode('utf-8'));
      finally:
        self.lock.release();

  def send_buffer_clear(self):
    if not self.lock.acquire(False):
      if (logger.isEnabledFor(logging.DEBUG)):
        logger.debug("Could not acquire civcom lock");
    else:
      try:
        del self.send_buffer[:];
      finally:
        self.lock.release();

  def get_send_result_string(self):
    result = "";
    self.pingstamp = time.time();  
    if not self.lock.acquire(False):
      if (logger.isEnabledFor(logging.DEBUG)):
        logger.debug("Could not acquire civcom lock");
    else:
      try:
        if len(self.send_buffer) > 0:
          result = "[" + ",".join(self.send_buffer) + "]";
        else:
          result = None;
      finally:
        del self.send_buffer[:];
        self.lock.release();
    return result;

  def send_error_to_client(self, message):
    if (logger.isEnabledFor(logging.ERROR)):
      logger.error(message);
    self.send_buffer_append(("{\"pid\":25,\"message\":\"" + message + "\"}").encode("utf-8"));
     
