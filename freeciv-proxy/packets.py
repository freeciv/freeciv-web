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

import generate_packets

from zerostrings import *
from BitVector import *
from struct import *

import json
from packetsizes import *
import logging
import math

packets = generate_packets.gen_main();

# Process packet from civserver 
def civserver_get_packet(type, payload):
  logging.debug("\nFull packet payload: \n " + repr(payload));
  result = {};
  if type not in packets.keys(): 
    logging.error("Invalid packet type");
    return None;
  packet_fields = packets[type].get_fields(); 
  curpos = 0;
  for i in range(0, len(packet_fields)):
    ftype = packet_fields[i]['type'];
    logging.debug("ftype:  " + ftype );

    if ftype == '$':
      # Parse bitvector
      logging.debug("path J");
      dx = struct.calcsize('>B');
      stype = packet_fields[i]['field'].struct_type;
      if not stype in packet_sizes:
          logging.error("Struct type not in packet_sizes :" + stype);
          continue;
         
      ssize = packet_sizes[stype] / 8;  # number of bytes in bitvector
      c_offset = 0;
      # FIXME: Cludge to make city_short_info and city_info packets work.
      if (type == 22): ssize += 3;
      if (type == 21): c_offset = -3;
      
      logging.debug(" dx: " + str(dx) + " iotype: " + packet_fields[i]['field'].dataio_type   
        + ", struct_type: " + packet_fields[i]['field'].struct_type  
        + ", payload:  " + repr(payload[curpos+c_offset:curpos+ssize+c_offset])  
        + ", ssize: " + str(ssize)  + "\n");
      if (ssize < 1): ssize = 1; 
      sub_res = [];
      for s in range(ssize):
          intVarByte = 0;
          if (dx == len(payload[curpos+c_offset:curpos+dx+c_offset])):
            intVarByte = unpackExt('>B', payload[curpos+c_offset:curpos+dx+c_offset])[0];
          else:
            logging.debug("ops, not enough info for bitvec. \n");              
          bv = BitVector( intVal = intVarByte, size = 8)
          for r in range(len(bv)):
            sub_res.append(bv[r]);
          curpos += dx; 
      #if (type == 21): sub_res = sub_res[5:];  #cludge for packet city_info  
    
    
    elif ftype == 'worklist':
      # parse a worklist packet.
      logging.debug("path H");
      logging.debug(" payload:  " + repr(payload[curpos:]) + "\n");
      dx = struct.calcsize('>?B');
      head_res = unpackExt('>?B', payload[curpos:curpos+dx]);  
      valid_worklist = head_res[0];
      len_worklist = head_res[1];
      logging.debug("valid: " + str(valid_worklist) + " length: " + str(len_worklist));
      if not valid_worklist:
        sub_res = "invalid";
      else :
        sub_res = [];
        curpos += dx;
        for s in range(len_worklist):
          sub_res[s] = {};
          head_res = unpackExt('>BB', payload[curpos:curpos+dx]);
          sub_res[s]["kind"] = head_res[0];
          sub_res[s]["identifier"] = head_res[1];
          dx = struct.calcsize('>BB');
          curpos += dx; 
      
    elif ftype != 'z' or (ftype == 'z' and packet_fields[i]['field'].is_array == 2):


      if packet_fields[i]['field'].is_array:
        # Determine array size
        array_size = 1;
        if packet_fields[i]['field'].is_array == 2:
          array_count_lookup = packet_fields[i]['field'].__dict__['array_size1_u'].replace("real_packet->", "");
        else:
          array_count_lookup = packet_fields[i]['field'].__dict__['array_size_u'].replace("real_packet->", "");
        if array_count_lookup in result:
          array_size = result[array_count_lookup];
        elif array_count_lookup in packet_sizes:
          array_size = packet_sizes[array_count_lookup];
        elif int(array_count_lookup) > 0:
          array_size = int(array_count_lookup);

        logging.debug("path D");
        sub_res = [];
        for s in xrange(array_size):
          if packet_fields[i]['field'].is_array == 2:
            # Unpacking two-dimensional array
            logging.debug("path G\n");
            logging.debug(" payload:  " + repr(payload[curpos:]) + "\n");
            sub_str = unpackExt('>'+ftype, payload[curpos:])[0]
            logging.debug("\nsub_str: " + sub_str + "\n");
            sub_res.append(sub_str);
            dx = len(sub_str) + 1;  
            curpos += dx;
          else:
            # Unpacking one-dimensional array.
            dx = struct.calcsize('>'+ftype);
            logging.debug(" dx: " + str(dx) + " payload:  " + repr(payload[curpos:curpos+dx]) + "\n");
            if (dx == len(payload[curpos:curpos+dx])): 
              sub_res.append(unpackExt('>'+ftype, payload[curpos:curpos+dx])[0]);
              curpos += dx;
            else:
              logging.debug("ops, not enough info\n");
              break;
        dx = 0;

      else:
        # Unpacking single C datatypes which are non-array. 
        logging.debug("path E");
        dx = struct.calcsize('>'+ftype);
        logging.debug(" dx: " + str(dx) + " iotype: " + packet_fields[i]['field'].dataio_type  
				+ ", struct_type: " + packet_fields[i]['field'].struct_type +
				", payload:  " + repr(payload[curpos:curpos+dx]) + "\n");

        if (dx == len(payload[curpos:curpos+dx])): 
          sub_res = unpackExt('>'+ftype, payload[curpos:curpos+dx])[0];
          if (math.isnan(sub_res)): sub_res = 0;  #nan isn't valid in JSON.   
        else:
          logging.debug("ops, not enough info\n");

    elif packet_fields[i]['name'] == 'inventions':
      logging.debug("path I.. ");
      logging.debug(" payload:  " + repr(payload[curpos:]) + "\n");

      array_size = 1;
      if packet_fields[i]['field'].is_array == 2:
        array_count_lookup = packet_fields[i]['field'].__dict__['array_size1_u'].replace("real_packet->", "");
      else:
        array_count_lookup = packet_fields[i]['field'].__dict__['array_size_u'].replace("real_packet->", "");
      if array_count_lookup in result:
        array_size = result[array_count_lookup];
      elif array_count_lookup in packet_sizes:
        array_size = packet_sizes[array_count_lookup];
      elif int(array_count_lookup) > 0:
        array_size = int(array_count_lookup);
      logging.debug("array_size: " + str(array_size));
      
      sub_res_str = unpackExt('>' + str(array_size) + 'c', payload[curpos:curpos+array_size]);
      # convert array from strings to ints.
      sub_res = map(lambda x: int(x), sub_res_str);
      dx = len(sub_res);

    elif ftype == 'z':
      # Unpacking a C string to Python String.
      logging.debug("path F");
      logging.debug(" payload:  " + repr(payload[curpos:]) + "\n");
      sub_res = unpackExt('>'+ftype, payload[curpos:])[0];  #FIXME: Her er noe galt!
      dx = len(sub_res) + 1;  

      # Check if string can be decoded to UTF-8.
      # Note that this is a cludge which conseals a real problem...
      try:
        test_decode = sub_res.decode("utf-8");
      except UnicodeDecodeError:
        logging.error("Unable to decode string to UTF-8");
        curpos += dx;
        continue;

    result[packet_fields[i]['name']] = sub_res; 
    logging.debug("** PART-RESULT:  " + packet_fields[i]['name'] + " : " + str(sub_res));
    curpos += dx;

  #logging.debug("JSON: " + json.dumps(result));
  result['packet_type'] = packets[type].name;
  return result; 

# Process packet from JSON to civserver.
def json_to_civserver(net_packet_json):
  if 'packet_type' in net_packet_json.keys(): 
    packet_type = net_packet_json['packet_type'];
  else:
    logging.error("Missing packet_type in packet.");
    return;

  res = "";

  packet_number = generate_packets.get_packet_name_type(packet_type);
  
  logging.debug("Processing packet from JSON to civserver: " + str(packet_number));

  if (packet_number == None): return None;
  for packet_label in packets[packet_number].get_fields():
    if packet_label['name'] not in net_packet_json:
      logging.error("Packet missing required field. ");
      return None;

    res += packExt('>'+packet_label['type'], net_packet_json[packet_label['name']]);

  header = packExt('>Hc', len(res)+3, chr(packet_number));
  res = header + res;
  #logging.info(repr(res));
  return res;
