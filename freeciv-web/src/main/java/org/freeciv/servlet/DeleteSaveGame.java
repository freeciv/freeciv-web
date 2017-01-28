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
import java.nio.file.Files;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.Properties;
import java.util.regex.Pattern;
import javax.naming.Context;
import javax.naming.InitialContext;
import javax.servlet.*;
import javax.servlet.http.*;
import javax.sql.DataSource;


/**
 * Deletes a savegame.
 *
 * URL: /deletesavegame
 */
public class DeleteSaveGame extends HttpServlet {
	private static final long serialVersionUID = 1L;
	private String PATTERN_VALIDATE_ALPHA_NUMERIC = "[0-9a-zA-Z\\.]*";
	private Pattern p = Pattern.compile(PATTERN_VALIDATE_ALPHA_NUMERIC);

	private String savegameDirectory;

	public void init(ServletConfig config) throws ServletException {
		super.init(config);

		try {
			Properties prop = new Properties();
			prop.load(getServletContext().getResourceAsStream("/WEB-INF/config.properties"));
			savegameDirectory = prop.getProperty("savegame_dir");
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public void doPost(HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {

		String username = request.getParameter("username");
		String savegame = request.getParameter("savegame");
		String password = java.net.URLDecoder.decode(request.getParameter("password"), "UTF-8");

		if (!p.matcher(username).matches() || username.toLowerCase().equals("pbem")) {
			response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
					"Invalid username");
			return;
		}
		if (savegame == null || savegame.length() > 100 || savegame.contains(".") || savegame.contains("/") || savegame.contains("\\")) {
			response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
					"Invalid savegame");
			return;
		}

		Connection conn = null;
		try {
			Context env = (Context) (new InitialContext().lookup("java:comp/env"));
			DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
			conn = ds.getConnection();

			String query =
					  "SELECT count(*) "
					+ "FROM auth "
					+ "WHERE LOWER(username) = LOWER(?) "
					+ "	AND password = MD5(?) "
					+ "	AND activated = '1' ";
			PreparedStatement preparedStatement = conn.prepareStatement(query);
			preparedStatement.setString(1, username);
			preparedStatement.setString(2, password);
			ResultSet rs = preparedStatement.executeQuery();
			rs.next();
			if (rs.getInt(1) != 1) {
				// user not found or deactivated
				return;
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

		try {
			File savegameFile = new File(savegameDirectory + username + "/" + savegame + ".sav.xz");
			if (savegameFile.exists() && savegameFile.isFile() && savegameFile.getName().endsWith(".sav.xz")) {
				Files.delete(savegameFile.toPath());
			}
		} catch (Exception err) {
			response.setHeader("result", "error");
			err.printStackTrace();
			response.sendError(HttpServletResponse.SC_BAD_REQUEST, "ERROR");
		}

	}

	public void doGet(HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {

		response.sendError(HttpServletResponse.SC_METHOD_NOT_ALLOWED, "This endpoint only supports the POST method.");

	}

}