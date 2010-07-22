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
from zerostrings import *
from packets import *
from threading import Thread, RLock;
import thread
import logging
import json
import time

HOST = 'localhost';
MAX_LEN_PACKET = 48;
VERSION = "+Freeciv.Devel.2009.Oct.18";  #must be kept in sync with Freeciv server.
VER_INFO = "-test";

class CivCom(Thread):

  def __init__ (self, username, civserverport, civserverhost):
    Thread.__init__(self)
    self.socket = None;
    self.username = username;
    self.civserverport = civserverport;
    self.civserverhost = civserverhost;
    self.key = username + str(civserverport) + civserverhost;
    self.send_buffer = [];
    self.lock = RLock();
    self.pingstamp = time.time();
    self.stopped = False;

    self.daemon = True;


  def set_civwebserver(self, civwebserver):
    self.civwebserver = civwebserver;


  def run(self):
    #setup connection to civserver
    logging.info("Start connection to civserver.")
    self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.socket.settimeout(1); 
    self.socket.setblocking(1);
    try:
      self.socket.connect((HOST, self.civserverport))
    except:
      self.send_error_to_client("Proxy unable to connect to civserver. Please go back and try again.");
      return;


    # send packet
    fmt = '>Hc'+str(len(self.username))+'sx'+str(len(VERSION))+'sx'+str(len(VER_INFO))+'s3I';
    packet = packExt(fmt, calcsize(fmt), chr(4), self.username, VERSION, VER_INFO, 2, 1, 99);
    self.socket.sendall(packet)

    #receive packets from server
    net_buf = "";
    while 1:
      net_buf = self.read_from_connection(net_buf);
    
      while 1:
          
        if (self.stopped):
          return;
              
        packet = self.get_packet_from_connection(net_buf);
        if (packet != None): 
          net_buf = net_buf[3+len(packet):];
          packet_payload = civserver_get_packet(self.packet_type, packet);
          
          self.civwebserver.buffer_send(packet_payload, self.key);
        else:
          break;


  def read_from_connection(self, net_buf):
    size = MAX_LEN_PACKET;
    #TODO: les saa mye som trengs...

    if (self.socket != None and self.key in self.civwebserver.civcoms.keys()):    
      data = self.socket.recv(size);

      # sleep a short while, to avoid excessive CPU use.
      if (len(data) == 0): 
        time.sleep(0.01);
          
      return net_buf + data;
    else:
      # sleep a short while, to avoid excessive CPU use.  
      time.sleep(0.01);
      return net_buf;

  def get_packet_from_connection(self, net_buf):

    if (len(net_buf) < 3): return None;

    result = unpackExt('>HB', net_buf[:3]);
    packet_len = result[0];
    self.packet_type = result[1];

    if (len(net_buf) < packet_len): return None;
    logging.debug("\nNEW PACKET:  type(" + str(self.packet_type) + ") len(" + str(packet_len) + ")" );
    return net_buf[3:packet_len];
  
  def close_connection(self):
    logging.error("Server connection closed. Removing civcom thread for " + self.username);
    if (self.key in self.civwebserver.civcoms.keys()):
      del self.civwebserver.civcoms[self.key];
    if (self.socket != None):
      self.socket.close();
      self.socket = None; 
    

  def send_packets_to_civserver(self, packets):
    json_packets = json.loads(packets);
    for packet in json_packets: 
      if not self.send_to_civserver(packet): return False;
    return True;

  def send_to_civserver(self, net_packet_json):
       
    civ_packet = json_to_civserver(net_packet_json);
    if not civ_packet == None:
      try:
        # Send packet to civserver
        self.socket.sendall(civ_packet)
        return True;
      except:
        send_error_to_client("Proxy unable to communicate with civserver.");
        return False;

    else:
      logging.error("invalid packet from 'json_to_civserver'");
      return False;

  def send_buffer_append(self, data):
    if not self.lock.acquire(False):
      logging.debug("Could not acquire civcom lock");
    else:
      try:
        self.send_buffer.append(data);
      finally:
        self.lock.release();

  def send_buffer_clear(self):
    if not self.lock.acquire(False):
      logging.debug("Could not acquire civcom lock");
    else:
      try:
        del self.send_buffer[:];
      finally:
        self.lock.release();

  def get_send_result_string(self):
    result = "";
    self.pingstamp = time.time();  
    if not self.lock.acquire(False):
      logging.debug("Could not acquire civcom lock");
    else:
      try:
        result = json.dumps(self.send_buffer, separators=(',',':'), allow_nan=False);
        del self.send_buffer[:];
      finally:
        self.lock.release();
    #logging.info("Sent to webclient: " + result);
    return result;

  def send_error_to_client(self, message):
    logging.error(message);
    msg = {};
    msg['packet_type'] = "packet_connect_msg";
    msg['message'] = message;
    self.civwebserver.buffer_send(msg, self.key);
      
