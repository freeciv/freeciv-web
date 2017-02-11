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
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.naming.Context;
import javax.naming.InitialContext;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.annotation.MultipartConfig;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.Part;
import javax.sql.DataSource;

import org.apache.commons.io.IOUtils;
import org.json.JSONObject;

/**
 * Displays the multiplayer games
 *
 * URL: /meta/metaserver.php
 */
@MultipartConfig
public class Metaserver extends HttpServlet {

	public class GameSummary {
		private String host;
		private int port;
		private String version;
		private String patches;
		private String state;
		private String message;
		private long duration;
		private String flag;
		private int turn;
		public int players;
		public String player;

		public long getDuration() {
			return duration;
		}

		public String getFlag() {
			return flag;
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

		public String getPlayer() {
			return player;
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
			if (message != null) {
				return message.contains("password-protected");
			}
			return false;
		}
	}

	private static final long serialVersionUID = 1L;
	private static final String CONTENT_TYPE = "application/json";

	private static final String INTERNAL_SERVER_ERROR = new JSONObject() //
			.put("statusCode", HttpServletResponse.SC_INTERNAL_SERVER_ERROR) //
			.put("error", "Internal server error.") //
			.toString();

	private static final String FORBIDDEN = new JSONObject() //
			.put("statusCode", HttpServletResponse.SC_FORBIDDEN) //
			.put("error", "Forbidden.") //
			.toString();

	private static final String BAD_REQUEST = new JSONObject() //
			.put("statusCode", HttpServletResponse.SC_BAD_REQUEST) //
			.put("error", "Bad Request.") //
			.toString();

	private static final List<String> SERVER_COLUMNS = new ArrayList<>();

	static {
		SERVER_COLUMNS.add("version");
		SERVER_COLUMNS.add("patches");
		SERVER_COLUMNS.add("capability");
		SERVER_COLUMNS.add("state");
		SERVER_COLUMNS.add("ruleset");
		SERVER_COLUMNS.add("message");
		SERVER_COLUMNS.add("type");
		SERVER_COLUMNS.add("available");
		SERVER_COLUMNS.add("humans");
		SERVER_COLUMNS.add("serverid");
		SERVER_COLUMNS.add("host");
		SERVER_COLUMNS.add("port");
	}

	@Override
	public void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {

		String query = null;
		Connection conn = null;
		PreparedStatement statement = null;
		ResultSet rs = null;
		try {
			Context env = (Context) (new InitialContext().lookup("java:comp/env"));
			DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
			conn = ds.getConnection();

			query = "SELECT COUNT(*) " //
					+ "FROM servers s " //
					+ "WHERE type = 'singleplayer' AND state = 'Running'";
			statement = conn.prepareStatement(query);
			rs = statement.executeQuery();
			rs.next();
			request.setAttribute("singlePlayerGames", rs.getInt(1));

			query = "SELECT COUNT(*) " //
					+ "FROM servers s " //
					+ "WHERE type = 'multiplayer' " //
					+ "	AND ( " //
					+ "		state = 'Running' OR " //
					+ "		(state = 'Pregame' " //
					+ "			AND CONCAT(s.host ,':',s.port) IN (SELECT hostport FROM players WHERE type <> 'A.I.')"
					+ "		)" //
					+ "	)";

			statement = conn.prepareStatement(query);
			rs = statement.executeQuery();
			rs.next();
			request.setAttribute("multiPlayerGames", rs.getInt(1));

			query = "SELECT host, port, version, patches, state, message, " //
					+ "	unix_timestamp()-unix_timestamp(stamp), " //
					+ "	IFNULL(" //
					+ "		(SELECT user FROM players p WHERE p.hostport = CONCAT(s.host ,':',s.port) AND p.type = 'Human' LIMIT 1 )," //
					+ " 	'none') AS player, " //
					+ "	IFNULL(" //
					+ "		(SELECT flag FROM players p WHERE p.hostport = CONCAT(s.host ,':',s.port) AND p.type = 'Human' LIMIT 1 ), " //
					+ "		'none') AS flag, " //
					+ "	(SELECT value " //
					+ "	   FROM variables " //
					+ "	  WHERE name = 'turn' " //
					+ "	    AND hostport = CONCAT(s.host ,':',s.port)) + 0 AS turn, " //
					+ "	(SELECT COUNT(*) " //
					+ "	   FROM players " //
					+ "	  WHERE hostport = CONCAT(s.host ,':',s.port)) AS players, " //
					+ "	(SELECT value " //
					+ "	   FROM variables " //
					+ "   WHERE name = 'turn' " //
					+ "	    AND hostport = CONCAT(s.host ,':',s.port)) + 0 AS turnsort " //
					+ "FROM servers s " //
					+ "WHERE type = 'singleplayer' " //
					+ "AND state = 'Running' " //
					+ "AND message NOT LIKE '%Multiplayer%' " //
					+ "ORDER BY turnsort DESC";

			statement = conn.prepareStatement(query);
			rs = statement.executeQuery();
			List<GameSummary> singlePlayerGames = new ArrayList<>();
			while (rs.next()) {
				GameSummary game = new GameSummary();
				game.host = rs.getString(1);
				game.port = rs.getInt(2);
				game.version = rs.getString(3);
				game.patches = rs.getString(4);
				game.state = rs.getString(5);
				game.message = rs.getString(6);
				game.duration = rs.getLong(7);
				game.player = rs.getString(8);
				game.flag = rs.getString(9);
				game.turn = rs.getInt(10);
				game.players = rs.getInt(11);
				singlePlayerGames.add(game);
			}
			request.setAttribute("singlePlayerGameList", singlePlayerGames);

			query = "SELECT host, port, version, patches, state, message, " //
					+ "UNIX_TIMESTAMP()-UNIX_TIMESTAMP(stamp), " //
					+ "	(SELECT COUNT(*) " //
					+ "	  FROM players " //
					+ "	 WHERE type = 'Human' " //
					+ "	   AND hostport = CONCAT(s.host ,':',s.port) " //
					+ "	) AS players," //
					+ "	(SELECT value " //
					+ "	  FROM variables " //
					+ "	  WHERE name = 'turn' " //
					+ "	    AND hostport = CONCAT(s.host ,':',s.port)" //
					+ "	) AS turn " //
					+ "FROM servers s " //
					+ "WHERE type = 'multiplayer' OR message LIKE '%Multiplayer%' " //
					+ "ORDER BY humans DESC, state DESC";

			statement = conn.prepareStatement(query);
			rs = statement.executeQuery();
			List<GameSummary> multiPlayerGames = new ArrayList<>();
			while (rs.next()) {
				GameSummary game = new GameSummary();
				game.host = rs.getString(1);
				game.port = rs.getInt(2);
				game.version = rs.getString(3);
				game.patches = rs.getString(4);
				game.state = rs.getString(5);
				game.message = rs.getString(6);
				game.duration = rs.getLong(7);
				game.players = rs.getInt(8);
				game.turn = rs.getInt(9);
				multiPlayerGames.add(game);
			}
			request.setAttribute("multiPlayerGamesList", multiPlayerGames);

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

		RequestDispatcher rd = request.getRequestDispatcher("metaserver.jsp");
		rd.forward(request, response);
	}

	@Override
	public void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {

		String localAddr = request.getLocalAddr();
		String remoteAddr = request.getRemoteAddr();

		if ((localAddr == null) || (remoteAddr == null) || !localAddr.equals(remoteAddr)) {
			response.setContentType(CONTENT_TYPE);
			response.setStatus(HttpServletResponse.SC_FORBIDDEN);
			response.getOutputStream().print(FORBIDDEN);
			return;
		}

		// Before Servlet 3.0, the Servlet API didn't natively support
		// multipart/form-data. It supports only the default form enctype of
		// application/x-www-form-urlencoded. The request.getParameter() and
		// consorts would all return null when using multipart form data.
		// @see:
		// http://stackoverflow.com/questions/2422468/how-to-upload-files-to-server-using-jsp-servlet
		String serverIsStopping = null;
		String sHost = null;
		String sPort = null;
		String dropPlayers = null;

		List<String> sPlUser = new ArrayList<String>();
		List<String> sPlName = new ArrayList<String>();
		List<String> sPlNation = new ArrayList<String>();
		List<String> sPlFlag = new ArrayList<String>();
		List<String> sPlType = new ArrayList<String>();
		List<String> sPlHost = new ArrayList<String>();
		List<String> variableNames = new ArrayList<String>();
		List<String> variableValues = new ArrayList<String>();

		Map<String, String> serverVariables = new HashMap<>();

		Collection<Part> parts = request.getParts();
		for (Part part : parts) {
			switch (part.getName()) {
			case "dropplrs":
				dropPlayers = IOUtils.toString(part.getInputStream());
				break;
			case "bye":
				serverIsStopping = IOUtils.toString(part.getInputStream());
				break;
			case "host":
				sHost = IOUtils.toString(part.getInputStream());
				serverVariables.put(part.getName(), sHost);
				break;
			case "port":
				sPort = IOUtils.toString(part.getInputStream());
				serverVariables.put(part.getName(), sPort);
				break;
			case "plu[]":
				sPlUser.add(IOUtils.toString(part.getInputStream()));
				break;
			case "pll[]":
				sPlName.add(IOUtils.toString(part.getInputStream()));
				break;
			case "pln[]":
				sPlNation.add(IOUtils.toString(part.getInputStream()));
				break;
			case "plf[]":
				sPlFlag.add(IOUtils.toString(part.getInputStream()));
				break;
			case "plt[]":
				sPlType.add(IOUtils.toString(part.getInputStream()));
				break;
			case "plh[]":
				sPlHost.add(IOUtils.toString(part.getInputStream()));
				break;
			case "vn[]":
				variableNames.add(IOUtils.toString(part.getInputStream()));
				break;
			case "vv[]":
				variableValues.add(IOUtils.toString(part.getInputStream()));
				break;
			default:
				if (SERVER_COLUMNS.contains(part.getName())) {
					serverVariables.put(part.getName(), IOUtils.toString(part.getInputStream()));
				}
				break;
			}
			IOUtils.closeQuietly(part.getInputStream());
		}

		int port = 0;
		try {
			port = Integer.parseInt(sPort);
			if ((port < 1024) || (port > 65535)) {
				throw new IllegalArgumentException("Invalid port supplied. Expected a number between 1024 and 65535");
			}
			if (sHost == null) {
				throw new IllegalArgumentException("Host parameter is required to perform this request.");
			}
		} catch (IllegalArgumentException e) {
			response.setContentType(CONTENT_TYPE);
			response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
			response.getOutputStream().print(BAD_REQUEST);
			return;
		}

		String hostPort = sHost + ':' + sPort;

		StringBuilder query = null;
		Connection conn = null;
		PreparedStatement statement = null;
		try {

			Context env = (Context) (new InitialContext().lookup("java:comp/env"));
			DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
			conn = ds.getConnection();

			if (serverIsStopping != null) {
				statement = conn.prepareStatement("DELETE FROM servers WHERE host = ? AND port = ?");
				statement.setString(1, sHost);
				statement.setInt(2, port);
				statement.executeUpdate();

				statement = conn.prepareStatement("DELETE FROM variables WHERE hostport = ?");
				statement.setString(1, hostPort);
				statement.executeUpdate();

				statement = conn.prepareStatement("DELETE FROM players WHERE hostport = ?");
				statement.setString(1, hostPort);
				statement.executeUpdate();
				return;
			}

			boolean isSettingPlayers = !sPlUser.isEmpty() && !sPlName.isEmpty() //
					&& !sPlNation.isEmpty() && !sPlFlag.isEmpty() //
					&& !sPlType.isEmpty() && !sPlHost.isEmpty();

			if (isSettingPlayers || (dropPlayers != null)) {

				statement = conn.prepareStatement("DELETE FROM players WHERE hostport= ?");
				statement.setString(1, hostPort);
				statement.executeUpdate();

				if (dropPlayers != null) {
					statement = conn.prepareStatement(
							"UPDATE servers SET available = 0, humans = -1 WHERE host = ? and port = ?");
					statement.setString(1, sHost);
					statement.setInt(2, port);
					statement.executeUpdate();
				}

				statement = conn.prepareStatement(
						"INSERT INTO players (hostport, user, name, nation, flag, type, host) VALUES (?, ?, ?, ?, ?, ?, ?)");

				try {

					for (int i = 0; i < sPlUser.size(); i++) {
						statement.setString(1, hostPort);
						statement.setString(2, sPlUser.get(i));
						statement.setString(3, sPlName.get(i));
						statement.setString(4, sPlNation.get(i));
						statement.setString(5, sPlFlag.get(i));
						statement.setString(6, sPlType.get(i));
						statement.setString(7, sPlHost.get(i));
						statement.executeUpdate();
					}

				} catch (IndexOutOfBoundsException e) {
					response.setContentType(CONTENT_TYPE);
					response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
					response.getOutputStream().print(BAD_REQUEST);
					return;
				}

			}

			// delete this variables that this server might have already set
			statement = conn.prepareStatement("DELETE FROM variables WHERE hostport = ?");
			statement.setString(1, hostPort);
			statement.executeUpdate();

			if ((!variableNames.isEmpty()) && (!variableValues.isEmpty())) {

				statement = conn.prepareStatement("INSERT INTO variables (hostport, name, value) VALUES (?, ?, ?)");

				try {
					for (int i = 0; i < variableNames.size(); i++) {
						statement.setString(1, hostPort);
						statement.setString(2, variableNames.get(i));
						statement.setString(3, variableValues.get(i));
						statement.executeUpdate();
					}
				} catch (IndexOutOfBoundsException e) {
					response.setContentType(CONTENT_TYPE);
					response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
					response.getOutputStream().print(BAD_REQUEST);
					return;
				}

			}

			statement = conn.prepareStatement("SELECT COUNT(*) FROM servers WHERE host = ? and port = ?");
			statement.setString(1, sHost);
			statement.setInt(2, port);
			ResultSet rs = statement.executeQuery();

			boolean serverExists = rs.next() && (rs.getInt(1) == 1);

			if (serverExists) {
				query = new StringBuilder("UPDATE servers SET ");
			} else {
				query = new StringBuilder("INSERT INTO servers SET ");
			}

			List<String> setServerVariables = new ArrayList<>(serverVariables.keySet());
			for (String parameter : setServerVariables) {
				query.append(parameter).append(" = ?, ");
			}
			query.append(" stamp = NOW()");

			if (serverExists) {
				query.append("WHERE host = ? AND port = ?");
			}

			statement = conn.prepareStatement(query.toString());
			int i = 1;
			for (String parameter : setServerVariables) {
				switch (parameter) {
				case "port":
				case "available":
					statement.setInt(i++, Integer.parseInt(serverVariables.get(parameter)));
					break;
				default:
					statement.setString(i++, serverVariables.get(parameter));
					break;
				}
			}
			if (serverExists) {
				statement.setString(i++, sHost);
				statement.setInt(i++, port);
			}
			statement.executeUpdate();

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