/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2015  The Freeciv-web project

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

***********************************************************************/


package org.freeciv.servlet;

import java.io.*;
import javax.servlet.*;
import javax.servlet.http.*;

import java.sql.*;

import java.util.regex.*;

import javax.sql.*;
import javax.naming.*;

import org.apache.commons.io.*;
import org.apache.commons.codec.digest.DigestUtils;
import org.apache.commons.codec.binary.Base64;

/*** The SaveServlet handles savegames, transfering them
  from the freeciv-web server to the localclient storage,
  saving SHA1 hash for security.
*/
public class SaveServlet extends HttpServlet {
	/**
	 * Serialization UID.
	 */
	private static final long serialVersionUID = 1L;
	private String PATTERN_VALIDATE_ALPHA_NUMERIC = "[0-9a-zA-Z\\.]*";
	private Pattern p = Pattern.compile(PATTERN_VALIDATE_ALPHA_NUMERIC);

	@SuppressWarnings("unchecked")
	public void doGet(HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {

		String username = "" + request.getParameter("username");
		if (!p.matcher(username).matches()) {
	    	   response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
				"Invalid username");
		    return;
		}

		String savename = "" + request.getParameter("savename");

		if (savename.length() == 0 || username.length() == 0 || savename.length() >= 64
		    || username.length() >= 32) {
	    	   response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
				"Invalid username or savename.");
		    return;
		}


		String relativeWebPath = "/savegames/" + username + ".sav.xz";
		String absoluteDiskPath = getServletContext().getRealPath(relativeWebPath).replaceAll("ROOT", "data");
		File file = new File(absoluteDiskPath);

		boolean found = false;
		for (int i = 0; i < 10; i++) {
			long fileAge = System.currentTimeMillis() - file.lastModified();
			if (file.exists() && fileAge > 400 && fileAge < 8000) {
				found = true;
				break;
			}
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}

		if (!found) {
	    	   response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
				"Savegame file not found!");
  		   return;

		} else if (file.length() == 0 || file.length() > 5000000) {
	    	   response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
				"Invalid file size of savegame.");
		   return;

		}

		InputStream in = new FileInputStream(file);
		byte[] compressedFile = IOUtils.toByteArray(in);
		String encodedFile = new String(Base64.encodeBase64(compressedFile));
		String savegameHash = DigestUtils.sha1Hex(username + savename + encodedFile);


		Connection conn = null;
		try {
		  Context env = (Context)(new InitialContext().lookup("java:comp/env"));
		  DataSource ds = (DataSource)env.lookup("jdbc/freeciv_mysql");
		  conn = ds.getConnection();

		  PreparedStatement stmt = conn.prepareStatement(
				"INSERT INTO savegames VALUES (NULL, ?,?,?)");
		  stmt.setString(1, username);
		  stmt.setString(2, savename);
		  stmt.setString(3, savegameHash);
		  stmt.executeUpdate();

		} catch (Exception err) {
		  err.printStackTrace();
		  response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
					"Savegame digest failed.");
		  return;

		} finally {
		   if (conn != null)
			try {
				conn.close();
			} catch (SQLException e) {
				e.printStackTrace();
			}
		}

		response.getOutputStream().print(encodedFile);
		in.close();

	}

}
