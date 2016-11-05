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
import java.nio.charset.Charset;
import org.apache.commons.codec.binary.Base64;

/*** The LoadServlet handles savegames, transfering them
  from the client's localstorage to freeciv-web server,
  saving SHA1 hash for security.
*/
@Deprecated
public class LoadServlet extends HttpServlet {
	/**
	 * Serialization UID.
	 */
	private static final long serialVersionUID = 1L;
	private String PATTERN_VALIDATE_ALPHA_NUMERIC = "[0-9a-zA-Z\\.]*";
	private Pattern p = Pattern.compile(PATTERN_VALIDATE_ALPHA_NUMERIC);

	@SuppressWarnings("unchecked")
	public void doPost(HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {


		InputStream in = request.getInputStream();

		String encodedFile = IOUtils.toString(in, Charset.forName("UTF-8"));
		byte[] compressedFile = Base64.decodeBase64(encodedFile);

		String savename = "" + request.getParameter("savename");
		String username = "" + request.getParameter("username");
		String savegameHash = DigestUtils.sha1Hex(username + savename + encodedFile);

		if (!p.matcher(username).matches()) {
	    	   response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
				"Invalid username");
		   return;
		}

		Connection conn = null;
		try {
		  Context env = (Context)(new InitialContext().lookup("java:comp/env"));
		  DataSource ds = (DataSource)env.lookup("jdbc/freeciv_mysql");
		  conn = ds.getConnection();

		  PreparedStatement stmt = conn.prepareStatement(
			"select count(*) from savegames where username = ? and title = ? and digest = ?");
		  stmt.setString(1, username);
		  stmt.setString(2, savename);
		  stmt.setString(3, savegameHash);

		  ResultSet rs = stmt.executeQuery();
		  if (rs.next()) {
			  if (rs.getInt(1) != 1) {
			    response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
				"Invalid savegame");
				return;
			  }
		  } else {
			  response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
				"Invalid savegame");
				return;
		  }

		  String relativeWebPath = "/savegames/" + username + ".sav.xz";
		  String absoluteDiskPath = getServletContext().getRealPath(relativeWebPath).replaceAll("ROOT", "data");
		  File file = new File(absoluteDiskPath);
		  FileUtils.writeByteArrayToFile(file, compressedFile);



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

		in.close();

		response.getOutputStream().print("success");

	}

}
