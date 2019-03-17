'''**********************************************************************
 Publite2 is a process manager which launches multiple Freeciv-web servers.
    Copyright (C) 2009-2019  The Freeciv-web project

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

import psutil

def get_pid_of_process(name, port):
  result = [];
  for proc in psutil.process_iter(attrs=['cmdline', 'name']):
    if proc.name() in str(name) and str(port) in " ".join(proc.cmdline()):
      result.append(str(proc.pid));
  return " ".join(result);
