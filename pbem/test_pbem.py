'''**********************************************************************
    Copyright (C) 2009-2016  The Freeciv-web project

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



import unittest
import mysql.connector
import pbem

# Unit tests from Freeciv-PBEM.
class TestPBEM(unittest.TestCase):

  def test_create_testusers(self):
    print("creating test users");
    result = None;
    cursor = None;
    cnx = None;
    try:
      url = urlparse(pbem.mysql_url)
      cnx = mysql.connector.connect(user=pbem.mysql_user, password=pbem.mysql_password, database=url.path[1:], host=url.hostname, port=url.port)
      cursor = cnx.cursor()
      query = ("insert ignore into auth (username, email, activated) values ('test', 'test@testtest.com', '1')");
      cursor.execute(query)
      query = ("insert ignore into auth (username, email, activated) values ('test2', 'test2@testtest.com', '1')");
      cursor.execute(query)
      cnx.commit()
    finally:
      cursor.close()
      cnx.close()
    

  def test_pbem_functions(self):
    print("testing pbem.");
    pbem.savedir = "test/";
    pbem.testmode = True;
    pbem.mail.testmode = True;
    pbem.process_savegames();
    pbem.process_ranklogs();
    pbem.remind_old_games();
    pbem.cleanup_expired_games();

if __name__ == '__main__':
    unittest.main()



