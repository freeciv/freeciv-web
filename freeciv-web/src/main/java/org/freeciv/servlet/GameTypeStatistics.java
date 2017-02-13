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
import java.sql.ResultSet;
import java.sql.SQLException;

import javax.naming.Context;
import javax.naming.InitialContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.sql.DataSource;

import org.json.JSONObject;

/**
 * Lists: game type statisticts
 *
 * URL: /meta/game-stats
 */
public class GameTypeStatistics extends HttpServlet {
	private static final long serialVersionUID = 1L;

	private static final String CONTENT_TYPE = "application/json";
	private static final String INTERNAL_SERVER_ERROR = new JSONObject() //
			.put("statusCode", HttpServletResponse.SC_INTERNAL_SERVER_ERROR) //
			.put("error", "Internal server error.") //
			.toString();

	@Override
	public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException, ServletException {

		Connection conn = null;

		try {
			Context env = (Context) (new InitialContext().lookup("java:comp/env"));
			DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
			conn = ds.getConnection();

			String query = "SELECT DISTINCT statsDate AS sd, "
					+ "(SELECT gameCount FROM games_played_stats WHERE statsDate = sd AND gameType = '0') AS web_single, "
					+ "(SELECT gameCount FROM games_played_stats WHERE statsDate = sd AND gameType = '1') AS web_multi, "
					+ "(SELECT gameCount FROM games_played_stats WHERE statsDate = sd AND gameType = '2') AS web_pbem, "
					+ "(SELECT gameCount FROM games_played_stats WHERE statsDate = sd AND gameType = '4') AS web_hotseat, "
					+ "(SELECT gameCount FROM games_played_stats WHERE statsDate = sd AND gameType = '3') AS desktop_multi, "
					+ "(SELECT gameCount FROM games_played_stats WHERE statsDate = sd AND gameType = '5') AS web_single_3d "
					+ "FROM games_played_stats";

			PreparedStatement preparedStatement = conn.prepareStatement(query);
			ResultSet rs = preparedStatement.executeQuery();
			StringBuilder result = new StringBuilder();
			while (rs.next()) {
				result //
						.append(rs.getInt(1)) //
						.append(',') //
						.append(rs.getInt(2)) //
						.append(',') //
						.append(rs.getInt(3)) //
						.append(',') //
						.append(rs.getInt(4)) //
						.append(',') //
						.append(rs.getInt(5)) //
						.append(',') //
						.append(rs.getInt(6)) //
						.append(',') //
						.append(rs.getInt(7)) //
						.append(';');

			}

			response.getOutputStream().print(result.toString());

		} catch (Exception err) {
			response.setContentType(CONTENT_TYPE);
			response.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
			response.getOutputStream().print(INTERNAL_SERVER_ERROR);
		} finally {
			if (conn != null) {
				try {
					conn.close();
				} catch (SQLException e) {
					e.printStackTrace();
				}
			}
		}
	}

}