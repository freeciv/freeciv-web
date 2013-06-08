# -*- coding: latin-1 -*-

''' 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
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
MAX_LEN_PACKET = 48;
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
    net_buf = bytearray(0);
    while 1:
      net_buf = self.read_from_connection(net_buf);
    
      while 1:
          
        if (self.stopped):
          return;
              
        packet = self.get_packet_from_connection(net_buf);
        if (packet != None): 
          net_buf = net_buf[4+len(packet):];
          result = packet[:-1];
          if (len(result) > 0):
            self.send_buffer_append(result);

        else:
          break;


  def read_from_connection(self, net_buf):
    size = MAX_LEN_PACKET;
    #TODO: les saa mye som trengs...

    if (self.socket != None and not self.stopped):    
      data = self.socket.recv(size);

      # sleep a short while, to avoid excessive CPU use.
      if (len(data) == 0): 
        time.sleep(0.005);
        return net_buf;
      else:
        return net_buf + data;
    else:
      # sleep a short while, to avoid excessive CPU use.  
      time.sleep(0.01);
      return net_buf;

  def get_packet_from_connection(self, net_buf):

    if (len(net_buf) < 4): return None;

    result = unpack('>HH', net_buf[:4]);
    packet_len = result[0];

    if (len(net_buf) < packet_len): return None;
    return net_buf[4:packet_len];

  
  def close_connection(self):
    if (logger.isEnabledFor(logging.ERROR)):
      logger.error("Server connection closed. Removing civcom thread for " + self.username);
    
    if (hasattr(self.civwebserver, "civcoms") and self.key in list(self.civwebserver.civcoms.keys())):
      del self.civwebserver.civcoms[self.key];
    
    if (self.socket != None):
      self.socket.close();
      self.socket = None; 
    

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
      return False;
    else:
      if (logger.isEnabledFor(logging.ERROR)):
        logger.error("invalid packet from 'json_to_civserver'");
      return False;

  def send_buffer_append(self, data):
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
    self.send_buffer_append("{\"pid\":18,\"message\":\"" + message + "\"}");
     
