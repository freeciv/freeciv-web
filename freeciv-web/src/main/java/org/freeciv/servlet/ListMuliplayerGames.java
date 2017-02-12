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
import java.util.ArrayList;
import java.util.List;

import javax.naming.Context;
import javax.naming.InitialContext;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.sql.DataSource;

import org.json.JSONObject;

/**
 * Displays the multiplayer games
 *
 * URL: /meta/fpmultimeta.php
 */
public class ListMuliplayerGames extends HttpServlet {

	// @TODO this will need to be moved elsewhere at some point
	public class GameSummary {

		private String host;
		private int port;
		private String version;
		private String patches;
		private String state;
		private String message;
		private long duration;
		private int players;
		private int turn;

		public long getDuration() {
			return duration;
		}

		public String getHost() {
			return host;
		}

		public String getMessage() {
			return message;
		}

		public String getPatches() {
			return patches;
		}

		public int getPlayers() {
			return players;
		}

		public int getPort() {
			return port;
		}

		public String getState() {
			return state;
		}

		public int getTurn() {
			return turn;
		}

		public String getVersion() {
			return version;
		}

		public boolean isProtected() {
			if (message == null) {
				return false;
			}
			return message.contains("password-protected");
		}
	}

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

			String query = "" //
					+ "  ( " //
					+ "    SELECT host, port, version, patches, state, message, " //
					+ "    UNIX_TIMESTAMP() - UNIX_TIMESTAMP(stamp), " //
					+ "    (SELECT value " //
					+ "       FROM variables " //
					+ "      WHERE name = 'turn' AND hostport = CONCAT(s.host, ':', s.port) " //
					+ "    ) AS turn, " //
					+ "    (SELECT COUNT(*) FROM players WHERE type = 'Human' AND hostport = CONCAT(s.host, ':', s.port)) AS players " //
					+ "    FROM servers s " //
					+ "    WHERE message NOT LIKE '%Private%' AND type = 'multiplayer' AND state = 'Running' ORDER BY state DESC " //
					+ "  ) " //
					+ "UNION " //
					+ "  ( " //
					+ "    SELECT host, port, version, patches, state, message, " //
					+ "    UNIX_TIMESTAMP() - UNIX_TIMESTAMP(stamp), " //
					+ "    (SELECT value " //
					+ "       FROM variables " //
					+ "      WHERE message NOT LIKE '%Private%' AND name = 'turn' AND hostport = CONCAT(s.host, ':', s.port) " //
					+ "    ) AS turn, " //
					+ "    (SELECT COUNT(*) FROM players WHERE type = 'Human' AND hostport = CONCAT(s.host, ':', s.port)) AS players " //
					+ "    FROM servers s " //
					+ "    WHERE message NOT LIKE '%Private%' " //
					+ "      AND type = 'multiplayer' " //
					+ "      AND state = 'Pregame' " //
					+ "      AND CONCAT(s.host ,':',s.port) IN (SELECT hostport FROM players WHERE type <> 'A.I.') " //
					+ "    LIMIT 1 " //
					+ "  ) " //
					+ "UNION " //
					+ "  ( " //
					+ "    SELECT host, port, version, patches, state, message, " //
					+ "    UNIX_TIMESTAMP()-UNIX_TIMESTAMP(stamp), " //
					+ "    (SELECT value " //
					+ "       FROM variables " //
					+ "      WHERE name = 'turn' AND hostport = CONCAT(s.host, ':', s.port) " //
					+ "    ) AS turn, " //
					+ "    (SELECT COUNT(*) FROM players WHERE type = 'Human' AND hostport = CONCAT(s.host, ':', s.port)) AS players " //
					+ "    FROM servers s " //
					+ "    WHERE type = 'multiplayer' AND state = 'Pregame' " //
					+ "    LIMIT 2 " //
					+ "  ) ";

			List<GameSummary> multiplayerGames = new ArrayList<>();
			PreparedStatement preparedStatement = conn.prepareStatement(query);
			ResultSet rs = preparedStatement.executeQuery();
			while (rs.next()) {
				GameSummary game = new GameSummary();
				game.host = rs.getString(1);
				game.port = rs.getInt(2);
				game.version = rs.getString(3);
				game.patches = rs.getString(4);
				game.state = rs.getString(5);
				game.message = rs.getString(6);
				game.duration = rs.getLong(7);
				game.turn = rs.getInt(8);
				game.players = rs.getInt(9);

				multiplayerGames.add(game);
			}

			request.setAttribute("games", multiplayerGames);

			RequestDispatcher rd = request.getRequestDispatcher("fpmultimeta.jsp");
			rd.forward(request, response);

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