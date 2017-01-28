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

import java.io.IOException;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.util.HashMap;
import java.util.Map;

import javax.naming.Context;
import javax.naming.InitialContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.sql.DataSource;

/**
 * This servlet will collect statistics about time played, and number of games stated.
 *
 * URL: /freeciv_time_played_stats
 */
public class FreecivStatsServlet extends HttpServlet {
	private static final long serialVersionUID = 1L;

	private final static Map<String, Integer> gameTypes = new HashMap<>();
	static {
		gameTypes.put("single2d", 0);
		gameTypes.put("single3d", 5);
		gameTypes.put("multi", 1);
		gameTypes.put("pbem", 2);
		gameTypes.put("hotseat", 4);
	}

	public void doPost(HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {

		Connection conn = null;
		try {

			String gameType = request.getParameter("type");
			if (!gameTypes.containsKey(gameType)) {
				return;
			}

			Context env = (Context) (new InitialContext().lookup("java:comp/env"));
			DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
			conn = ds.getConnection();

			int gameTypeId = gameTypes.get(gameType);

			String insert =
							  "INSERT INTO games_played_stats (statsDate, gameType, gameCount) "
							+ "VALUES (CURDATE(), ?, 1) "
							+ "ON DUPLICATE KEY UPDATE gameCount = gameCount + 1";
			PreparedStatement preparedStatement = conn.prepareStatement(insert);
			preparedStatement.setInt(1, gameTypeId);
			preparedStatement.executeUpdate();


		} catch (Exception err) {
			System.err.println("Error in FreecivStatsServlet" + err.getMessage());
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