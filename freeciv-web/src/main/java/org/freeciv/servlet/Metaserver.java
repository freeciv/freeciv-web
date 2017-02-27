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
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.naming.Context;
import javax.naming.InitialContext;
import javax.servlet.ServletException;
import javax.servlet.annotation.MultipartConfig;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.sql.DataSource;

import org.json.JSONObject;

/**
 * Displays the multiplayer games
 *
 * URL: /meta/metaserver
 */
@MultipartConfig
public class Metaserver extends HttpServlet {

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
	public void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {

		String localAddr = request.getLocalAddr();
		String remoteAddr = request.getRemoteAddr();

		if ((localAddr == null) || (remoteAddr == null) || !localAddr.equals(remoteAddr)) {
			response.setContentType(CONTENT_TYPE);
			response.setStatus(HttpServletResponse.SC_FORBIDDEN);
			response.getOutputStream().print(FORBIDDEN);
			return;
		}

		String serverIsStopping = request.getParameter("bye");
		String sHost = request.getParameter("host");
		String sPort = request.getParameter("port");
		String dropPlayers = request.getParameter("dropplrs");

		List<String> sPlUser = request.getParameterValues("plu[]") == null ? null
				: Arrays.asList(request.getParameterValues("plu[]"));
		List<String> sPlName = request.getParameterValues("pll[]") == null ? null
				: Arrays.asList(request.getParameterValues("pll[]"));
		List<String> sPlNation = request.getParameterValues("pln[]") == null ? null
				: Arrays.asList(request.getParameterValues("pln[]"));
		List<String> sPlFlag = request.getParameterValues("plf[]") == null ? null
				: Arrays.asList(request.getParameterValues("plf[]"));
		List<String> sPlType = request.getParameterValues("plt[]") == null ? null
				: Arrays.asList(request.getParameterValues("plt[]"));
		List<String> sPlHost = request.getParameterValues("plh[]") == null ? null
				: Arrays.asList(request.getParameterValues("plh[]"));
		List<String> variableNames = request.getParameterValues("vn[]") == null ? null
				: Arrays.asList(request.getParameterValues("vn[]"));
		List<String> variableValues = request.getParameterValues("vv[]") == null ? null
				: Arrays.asList(request.getParameterValues("vv[]"));

		Map<String, String> serverVariables = new HashMap<>();
		for (String serverParameter : SERVER_COLUMNS) {
			String parameter = request.getParameter(serverParameter);
			if (parameter != null) {
				serverVariables.put(serverParameter, parameter);
			}
		}

		// Data validation
		String query;
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
			response.setContentType(CONTENT_TYPE);
			response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
			response.getOutputStream().print(BAD_REQUEST);
			return;
		}

		String hostPort = sHost + ':' + sPort;
		Connection conn = null;
		PreparedStatement statement;
		try {

			Context env = (Context) (new InitialContext().lookup("java:comp/env"));
			DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
			conn = ds.getConnection();

			if (serverIsStopping != null) {
				query = "DELETE FROM servers WHERE host = ? AND port = ?";
				statement = conn.prepareStatement(query);
				statement.setString(1, sHost);
				statement.setInt(2, port);
				statement.executeUpdate();

				query = "DELETE FROM variables WHERE hostport = ?";
				statement = conn.prepareStatement(query);
				statement.setString(1, hostPort);
				statement.executeUpdate();

				query = "DELETE FROM players WHERE hostport = ?";
				statement = conn.prepareStatement(query);
				statement.setString(1, hostPort);
				statement.executeUpdate();
				return;
			}

			boolean isSettingPlayers = (sPlUser != null) && !sPlUser.isEmpty() //
					&& (sPlName != null) && !sPlName.isEmpty() //
					&& (sPlNation != null) && !sPlNation.isEmpty() //
					&& (sPlFlag != null) && !sPlFlag.isEmpty() //
					&& (sPlType != null) && !sPlType.isEmpty() //
					&& (sPlHost != null) && !sPlHost.isEmpty();

			if (isSettingPlayers || (dropPlayers != null)) {
				query = "DELETE FROM players WHERE hostport = ?";
				statement = conn.prepareStatement(query);
				statement.setString(1, hostPort);
				statement.executeUpdate();

				if (dropPlayers != null) {
					query = "UPDATE servers SET available = 0, humans = -1 WHERE host = ? AND port = ?";
					statement = conn.prepareStatement(query);
					statement.setString(1, sHost);
					statement.setInt(2, port);
					statement.executeUpdate();
				}

				if (isSettingPlayers) {

					query = "INSERT INTO players (hostport, user, name, nation, flag, type, host) VALUES (?, ?, ?, ?, ?, ?, ?)";
					statement = conn.prepareStatement(query);
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
			}

			// delete this variables that this server might have already set
			statement = conn.prepareStatement("DELETE FROM variables WHERE hostport = ?");
			statement.setString(1, hostPort);
			statement.executeUpdate();

			if ((!variableNames.isEmpty()) && (!variableValues.isEmpty())) {

				query = "INSERT INTO variables (hostport, name, value) VALUES (?, ?, ?)";
				statement = conn.prepareStatement(query);
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

			query = "SELECT COUNT(*) FROM servers WHERE host = ? and port = ?";
			statement = conn.prepareStatement(query);
			statement.setString(1, sHost);
			statement.setInt(2, port);
			ResultSet rs = statement.executeQuery();

			boolean serverExists = rs.next() && (rs.getInt(1) == 1);

			List<String> setServerVariables = new ArrayList<>(serverVariables.keySet());
			StringBuilder queryBuilder = new StringBuilder();
			if (serverExists) {
				queryBuilder.append("UPDATE servers SET ");
				for (String parameter : setServerVariables) {
					queryBuilder.append(parameter).append(" = ?, ");
				}
				queryBuilder.append(" stamp = NOW() ");
				queryBuilder.append(" WHERE host = ? AND port = ?");
			} else {
				queryBuilder = new StringBuilder("INSERT INTO servers SET ");
				for (String parameter : setServerVariables) {
					queryBuilder.append(parameter).append(" = ?, ");
				}
				queryBuilder.append(" stamp = NOW() ");
			}

			query = queryBuilder.toString();
			statement = conn.prepareStatement(query);
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