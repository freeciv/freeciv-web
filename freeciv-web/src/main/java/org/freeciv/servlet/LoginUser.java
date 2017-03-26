/*******************************************************************************
 * Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
 * Copyright (C) 2009-2017 The Freeciv-web project
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/
package org.freeciv.servlet;

import java.io.*;
import javax.servlet.*;
import javax.servlet.http.*;

import java.sql.*;

import javax.sql.*;
import javax.naming.*;


/**
 * Validates that the give username and password match
 * a user in the database.
 * Such user must be activated.
 *
 * URL: /login_user
 */
public class LoginUser extends HttpServlet {
	private static final long serialVersionUID = 1L;

	public void doPost(HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {

		String username = java.net.URLDecoder.decode(request.getParameter("username"), "UTF-8");
		String secure_password = java.net.URLDecoder.decode(request.getParameter("sha_password"), "UTF-8");
		/* TODO: Remove md5 hashed password once enough users have migrated to SHA. */
		@Deprecated String old_md5_password = java.net.URLDecoder.decode(request.getParameter("password"), "UTF-8");


		if (old_md5_password == null || old_md5_password.length() <= 2) {
			response.sendError(HttpServletResponse.SC_BAD_REQUEST,
					"Invalid password. Please try again with another password.");
			return;
		}
		if (secure_password == null || secure_password.length() <= 2) {
			response.sendError(HttpServletResponse.SC_BAD_REQUEST,
					"Invalid password. Please try again with another password.");
			return;
		}
		if (username == null || username.length() <= 2) {
			response.sendError(HttpServletResponse.SC_BAD_REQUEST,
					"Invalid username. Please try again with another username.");
			return;
		}


		Connection conn = null;
		try {
			Context env = (Context) (new InitialContext().lookup("java:comp/env"));
			DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
			conn = ds.getConnection();

			String query =
					  "SELECT username, password IS NOT NULL "
					+ "FROM auth "
					+ "WHERE LOWER(username) = LOWER(?) "
					+ "	AND ((password = ? AND password IS NOT NULL) OR (secure_password = ? AND secure_password IS NOT NULL)) "
					+ "	AND activated = '1' LIMIT 1";

			PreparedStatement preparedStatement = conn.prepareStatement(query);
			preparedStatement.setString(1, username);
			preparedStatement.setString(2, old_md5_password);
			preparedStatement.setString(3, secure_password);
			ResultSet rs = preparedStatement.executeQuery();

			if (!rs.next()) {
				response.getOutputStream().print("Failed");
			} else {
				if (!rs.getString(1).equals(username)) {
					response.getOutputStream().print("Failed");
					return;
				}

				// Login is OK!
				response.getOutputStream().print("OK");

				if (rs.getInt(2) == 1) {
					// migrate user to SHA based password.
					String migrate = "UPDATE auth SET password = NULL, secure_password = ? WHERE username = ? and password = ?";
					PreparedStatement migrateStatement = conn.prepareStatement(migrate);
					migrateStatement.setString(1, secure_password);
					migrateStatement.setString(2, username);
					migrateStatement.setString(3, old_md5_password);
					migrateStatement.executeUpdate();

				}
			}

		} catch (Exception err) {
			response.setHeader("result", "error");
			err.printStackTrace();
			response.sendError(HttpServletResponse.SC_BAD_REQUEST, "Unable to login");
		} finally {
			if (conn != null)
				try {
					conn.close();
				} catch (SQLException e) {
					e.printStackTrace();
				}
		}
	}

	public void doGet(HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {

		response.sendError(HttpServletResponse.SC_METHOD_NOT_ALLOWED, "This endpoint only supports the POST method.");

	}

}