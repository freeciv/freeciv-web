/********************************************************************** 
 Freeciv - Copyright (C) 2013 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/


package org.freeciv.servlet;

import java.security.MessageDigest;

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

/*** The SaveServlet handles savegames, transfering them
  from the freeciv-web server to the localclient storage,
  saving SHA1 hash for security.
*/
public class SaveServlet extends HttpServlet {
	/**
	 * Serialization UID.
	 */
	private static final long serialVersionUID = 1L;
	String PATTERN_VALIDATE_ALPHA_NUMERIC = "[0-9a-zA-Z\\.]*";
	Pattern p = Pattern.compile(PATTERN_VALIDATE_ALPHA_NUMERIC);

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

		String relativeWebPath = "/savegames/" + username + ".sav.bz2";
		String absoluteDiskPath = getServletContext().getRealPath(relativeWebPath);
		File file = new File(absoluteDiskPath);
		if (!file.exists()) {
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
		String savegameHash = DigestUtils.shaHex(username + savename + encodedFile);


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
