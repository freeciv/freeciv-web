/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

var connections = {};
var conn_ping_info = {};
var debug_ping_list = [];

function find_conn_by_id(id)
{
  return connections[id];
}


function client_remove_cli_conn(connection)
{
  delete connections[connection['id']];
}

function conn_list_append(connection)
{
  connections[connection['id']] = connection;
}
