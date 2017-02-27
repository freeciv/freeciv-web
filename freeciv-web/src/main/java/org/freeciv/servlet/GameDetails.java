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
import javax.servlet.annotation.MultipartConfig;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.sql.DataSource;

/**
 * Displays detailed information about a specific game
 *
 * URL: /meta/game-details
 */
@MultipartConfig
public class GameDetails extends HttpServlet {

	public class PlayerSummary {
		private String flag;
		private String name;
		private String nation;
		private String user;
		private String type;

		public String getFlag() {
			return flag;
		}

		public String getName() {
			return name;
		}

		public String getNation() {
			return nation;
		}

		public String getType() {
			return type;
		}

		public String getUser() {
			return user;
		}
	}

	public class VariableSummary {
		private String name;
		private String value;

		public String getName() {
			return name;
		}

		public String getValue() {
			return value;
		}

	}

	private static final long serialVersionUID = 1L;

	@Override
	public void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {

		String sHost = request.getParameter("host");
		String sPort = request.getParameter("port");

		int port;
		try {
			if (sPort == null) {
				throw new IllegalArgumentException("Port must be supplied.");
			}
			port = Integer.parseInt(sPort);
			if ((port < 1024) || (port > 65535)) {
				throw new IllegalArgumentException("Invalid port supplied. Expected a number between 1024 and 65535");
			}
			if (sHost == null) {
				throw new IllegalArgumentException("Host parameter is required to perform this request.");
			}
		} catch (IllegalArgumentException e) {
			RequestDispatcher rd = request.getRequestDispatcher("game-details.jsp");
			rd.forward(request, response);
			return;
		}

		String hostPort = sHost + ':' + sPort;
		String query;
		Connection conn = null;
		PreparedStatement statement = null;
		ResultSet rs = null;
		try {
			Context env = (Context) (new InitialContext().lookup("java:comp/env"));
			DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
			conn = ds.getConnection();

			query = "SELECT * FROM servers WHERE host = ? AND port = ?";

			statement = conn.prepareStatement(query);
			statement.setString(1, sHost);
			statement.setInt(2, port);
			rs = statement.executeQuery();
			if (rs.next()) {
				request.setAttribute("version", rs.getString("version"));
				request.setAttribute("patches", rs.getString("patches"));
				request.setAttribute("capability", rs.getString("capability"));
				request.setAttribute("state", rs.getString("state"));
				request.setAttribute("ruleset", rs.getString("ruleset"));
				request.setAttribute("serverid", rs.getString("serverid"));
				request.setAttribute("port", port);
				request.setAttribute("host", sHost);
			} else {
				RequestDispatcher rd = request.getRequestDispatcher("game-information.jsp");
				rd.forward(request, response);
				return;
			}

			query = "SELECT * FROM players WHERE hostport = ? ORDER BY name";
			statement = conn.prepareStatement(query);
			statement.setString(1, hostPort);
			rs = statement.executeQuery();
			List<PlayerSummary> players = new ArrayList<>();
			while (rs.next()) {
				PlayerSummary player = new PlayerSummary();
				player.flag = rs.getString("flag");
				player.name = rs.getString("name");
				player.nation = rs.getString("nation");
				player.user = rs.getString("user");
				player.type = rs.getString("type");
				players.add(player);
			}
			request.setAttribute("players", players);

			query = "SELECT * FROM variables WHERE hostport = ? ORDER BY name";
			statement = conn.prepareStatement(query);
			statement.setString(1, hostPort);
			rs = statement.executeQuery();
			List<VariableSummary> variables = new ArrayList<>();
			while (rs.next()) {
				VariableSummary variable = new VariableSummary();
				variable.name = rs.getString("name");
				variable.value = rs.getString("value");
				variables.add(variable);
			}
			request.setAttribute("variables", variables);

		} catch (Exception err) {
			request.removeAttribute("state");
			RequestDispatcher rd = request.getRequestDispatcher("/WEB-INF/jsp/game/details.jsp");
			rd.forward(request, response);
			return;
		} finally {
			if (conn != null) {
				try {
					conn.close();
				} catch (SQLException e) {
					e.printStackTrace();
				}
			}
		}

		RequestDispatcher rd = request.getRequestDispatcher("/WEB-INF/jsp/game/details.jsp");
		rd.forward(request, response);
	}
}