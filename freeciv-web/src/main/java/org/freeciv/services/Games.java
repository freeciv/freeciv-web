package org.freeciv.services;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

import javax.naming.Context;
import javax.naming.InitialContext;
import javax.naming.NamingException;
import javax.sql.DataSource;

import org.freeciv.model.Game;

public class Games {

	public int getMultiPlayerCount() {

		String query;
		Connection connection = null;
		PreparedStatement statement = null;
		ResultSet rs = null;
		try {
			Context env = (Context) (new InitialContext().lookup("java:comp/env"));
			DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
			connection = ds.getConnection();

			query = "SELECT COUNT(*) AS count " //
					+ "FROM servers s " //
					+ "WHERE type IN ('multiplayer', 'longturn') " //
					+ "	AND ( " //
					+ "		state = 'Running' OR " //
					+ "		(state = 'Pregame' " //
					+ "			AND CONCAT(s.host ,':',s.port) IN (SELECT hostport FROM players WHERE type <> 'A.I.')"
					+ "		)" //
					+ "	)";

			statement = connection.prepareStatement(query);
			rs = statement.executeQuery();
			rs.next();
			return rs.getInt("count");
		} catch (SQLException e) {
			throw new RuntimeException(e);
		} catch (NamingException e) {
			throw new RuntimeException(e);
		} finally {
			if (connection != null) {
				try {
					connection.close();
				} catch (SQLException e) {
					e.printStackTrace();
				}
			}
		}
	}

	public List<Game> getMultiPlayerGames() {

		String query;
		Connection connection = null;
		PreparedStatement statement = null;
		ResultSet rs = null;
		try {
			Context env = (Context) (new InitialContext().lookup("java:comp/env"));
			DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
			connection = ds.getConnection();

			query = "SELECT host, port, version, patches, state, message, " //
					+ "UNIX_TIMESTAMP()-UNIX_TIMESTAMP(stamp) AS duration, " //
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
					+ "WHERE type IN ('multiplayer', 'longturn') OR message LIKE '%Multiplayer%' " //
					+ "ORDER BY humans DESC, state DESC";

			statement = connection.prepareStatement(query);
			rs = statement.executeQuery();
			List<Game> multiPlayerGames = new ArrayList<>();
			while (rs.next()) {
				Game game = new Game() //
						.setHost(rs.getString("host")) //
						.setPort(rs.getInt("port")) //
						.setVersion(rs.getString("version")) //
						.setPatches(rs.getString("patches")) //
						.setState(rs.getString("state")) //
						.setMessage(rs.getString("message")) //
						.setDuration(rs.getLong("duration")) //
						.setTurn(rs.getInt("turn")) //
						.setPlayers(rs.getInt("players"));
				multiPlayerGames.add(game);
			}
			return multiPlayerGames;
		} catch (SQLException e) {
			throw new RuntimeException(e);
		} catch (NamingException e) {
			throw new RuntimeException(e);
		} finally {
			if (connection != null) {
				try {
					connection.close();
				} catch (SQLException e) {
					e.printStackTrace();
				}
			}
		}
	}

	public int getSinglePlayerCount() {

		String query;
		Connection connection = null;
		PreparedStatement statement = null;
		ResultSet rs = null;
		try {
			Context env = (Context) (new InitialContext().lookup("java:comp/env"));
			DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
			connection = ds.getConnection();
			query = "SELECT COUNT(*) AS count " //
					+ "FROM servers s " //
					+ "WHERE type = 'singleplayer' AND state = 'Running'";
			statement = connection.prepareStatement(query);
			rs = statement.executeQuery();
			rs.next();
			return rs.getInt("count");
		} catch (SQLException e) {
			throw new RuntimeException(e);
		} catch (NamingException e) {
			throw new RuntimeException(e);
		} finally {
			if (connection != null) {
				try {
					connection.close();
				} catch (SQLException e) {
					e.printStackTrace();
				}
			}
		}
	}

	public List<Game> getSinglePlayerGames() {

		String query;
		Connection connection = null;
		PreparedStatement statement = null;
		ResultSet rs = null;
		try {
			Context env = (Context) (new InitialContext().lookup("java:comp/env"));
			DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
			connection = ds.getConnection();
			query = "SELECT host, port, version, patches, state, message, " //
					+ "	unix_timestamp()-unix_timestamp(stamp) AS duration, " //
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

			statement = connection.prepareStatement(query);
			rs = statement.executeQuery();
			List<Game> singlePlayerGames = new ArrayList<>();
			while (rs.next()) {
				Game game = new Game() //
						.setHost(rs.getString("host")) //
						.setPort(rs.getInt("port")) //
						.setVersion(rs.getString("version")) //
						.setPatches(rs.getString("patches")) //
						.setState(rs.getString("state")) //
						.setMessage(rs.getString("message")) //
						.setDuration(rs.getLong("duration")) //
						.setTurn(rs.getInt("turn")) //
						.setPlayers(rs.getInt("players")) //
						.setPlayer(rs.getString("player")) //
						.setFlag(rs.getString("flag"));

				singlePlayerGames.add(game);
			}
			return singlePlayerGames;
		} catch (SQLException e) {
			throw new RuntimeException(e);
		} catch (NamingException e) {
			throw new RuntimeException(e);
		} finally {
			if (connection != null) {
				try {
					connection.close();
				} catch (SQLException e) {
					e.printStackTrace();
				}
			}
		}

	}

	public List<Game> summary() {

		Connection connection = null;
		try {
			Context env = (Context) (new InitialContext().lookup("java:comp/env"));
			DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
			connection = ds.getConnection();

			String query = "" //
					+ "  ( " //
					+ "    SELECT host, port, version, patches, state, message, " //
					+ "    UNIX_TIMESTAMP() - UNIX_TIMESTAMP(stamp) AS duration, " //
					+ "    (SELECT value " //
					+ "       FROM variables " //
					+ "      WHERE name = 'turn' AND hostport = CONCAT(s.host, ':', s.port) " //
					+ "    ) AS turn, " //
					+ "    (SELECT COUNT(*) FROM players WHERE type = 'Human' AND hostport = CONCAT(s.host, ':', s.port)) AS players " //
					+ "    FROM servers s " //
					+ "    WHERE message NOT LIKE '%Private%' AND type IN ('multiplayer', 'longturn') AND state = 'Running' ORDER BY state DESC " //
					+ "  ) " //
					+ "UNION " //
					+ "  ( " //
					+ "    SELECT host, port, version, patches, state, message, " //
					+ "    UNIX_TIMESTAMP() - UNIX_TIMESTAMP(stamp) AS duration, " //
					+ "    (SELECT value " //
					+ "       FROM variables " //
					+ "      WHERE message NOT LIKE '%Private%' AND name = 'turn' AND hostport = CONCAT(s.host, ':', s.port) " //
					+ "    ) AS turn, " //
					+ "    (SELECT COUNT(*) FROM players WHERE type = 'Human' AND hostport = CONCAT(s.host, ':', s.port)) AS players " //
					+ "    FROM servers s " //
					+ "    WHERE message NOT LIKE '%Private%' " //
					+ "      AND type IN ('multiplayer', 'longturn') " //
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
					+ "    WHERE type IN ('multiplayer', 'longturn') AND state = 'Pregame' " //
					+ "    LIMIT 2 " //
					+ "  ) ";

			List<Game> multiplayerGames = new ArrayList<>();
			PreparedStatement preparedStatement = connection.prepareStatement(query);
			ResultSet rs = preparedStatement.executeQuery();
			while (rs.next()) {
				multiplayerGames.add(new Game() //
						.setHost(rs.getString("host")) //
						.setPort(rs.getInt("port")) //
						.setVersion(rs.getString("version")) //
						.setPatches(rs.getString("patches")) //
						.setState(rs.getString("state")) //
						.setMessage(rs.getString("message")) //
						.setDuration(rs.getLong("duration")) //
						.setTurn(rs.getInt("turn")) //
						.setPlayers(rs.getInt("players")) //
				);
			}

			return multiplayerGames;

		} catch (SQLException e) {
			throw new RuntimeException(e);
		} catch (NamingException e) {
			throw new RuntimeException(e);
		} finally {
			if (connection != null) {
				try {
					connection.close();
				} catch (SQLException e) {
					e.printStackTrace();
				}
			}
		}
	}

}
