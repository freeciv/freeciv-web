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

import hashlib,base64,random
import functools

# TODO: Make the username / password configurabe.
API_KEYS = {
	'test': 'test'
}


def api_auth(username, password):
	if username in API_KEYS:
		return True
	return False

def basic_auth(auth):
	def decore(f):
		def _request_auth(handler):
			handler.set_header('WWW-Authenticate', 'Basic realm=JSL')
			handler.set_status(401)
			handler.finish()
			return False
		
		@functools.wraps(f)
		def new_f(*args):
			handler = args[0]
 
			auth_header = handler.request.headers.get('Authorization')
			if auth_header is None: 
				return _request_auth(handler)
			if not auth_header.startswith('Basic '): 
				return _request_auth(handler)
 
			auth_decoded = base64.b64decode(auth_header[6:]).decode('ascii')
			username, password = str(auth_decoded).split(':', 1)
 
			if (auth(username, password)):
				f(*args)
			else:
				_request_auth(handler)
					
		return new_f
	return decore
