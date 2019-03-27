package org.freeciv.persistence;

import java.util.List;

public class DbManager {

	public static String getQuerySelectPort() {
		return " SELECT port FROM servers " //
				+ " WHERE state = 'Pregame' AND type = ? AND humans = '0' " //
				+ " ORDER BY RAND() " //
				+ " LIMIT 1 ";
	}

	public static String getQueryCountServers() {
		return " SELECT COUNT(*) FROM servers " //
				+ " WHERE port = ? ";
	}

	public static String getQueryCountServersByHost() {
		return " SELECT COUNT(*) FROM servers " //
				+ " WHERE host = ? and port = ? ";
	}

	public static String getQuerySaltHash() {
		return " SELECT secure_hashed_password FROM auth " //
				+ " WHERE LOWER(username) = LOWER(?) AND activated = '1' " //
				+ " LIMIT 1 ";
	}

	public static String getQueryCountUsers() {
		return "SELECT COUNT(*) FROM auth ";
	}

	public static String getQuerySelectUserById() {
		return " SELECT username " //
				+ " FROM auth " //
				+ " WHERE activated='1' " //
				+ "	AND id >= (SELECT FLOOR(MAX(id) * RAND()) FROM `auth`) " //
				+ " ORDER BY id LIMIT 1 ";
	}

	public static String getQuerySelectUser() {
		return "SELECT username, activated " //
				+ " FROM auth " //
				+ " WHERE LOWER(username) = LOWER(?) " //
				+ "	OR LOWER(email) = LOWER(?)";
	}

	public static String getQueryUpdateAuthDeactivate() {
		return " UPDATE auth SET activated = '0' " //
				+ " WHERE username = ? ";
	}

	public static String getQueryInsertStats() {
		return " INSERT INTO games_played_stats (statsDate, gameType, gameCount) " //
				+ " VALUES (CURDATE(), ?, 1) " //
				+ " ON DUPLICATE KEY UPDATE gameCount = gameCount + 1 ";
	}

	public static String getQuerySelectServers() {
		return " SELECT * FROM servers " //
				+ "WHERE host = ? AND port = ? ";
	}

	public static String getQuerySelectPlayers() {
		return " SELECT * FROM players " //
				+ " WHERE hostport = ? " //
				+ " ORDER BY name";
	}

	public static String getQuerySelectVariables() {
		return " SELECT * FROM variables " //
				+ " WHERE hostport = ? " //
				+ " ORDER BY name";
	}

	public static String getQuerySelectGameResults() {
		return " SELECT winner AS player, COUNT(winner) AS wins " //
				+ "  FROM game_results " //
				+ " GROUP BY winner " //
				+ " ORDER BY wins DESC " //
				+ " LIMIT 10 ";
	}

	public static String getQuerySelectStats() {
		return " SELECT " //
				+ "	(SELECT COUNT(*) FROM servers WHERE state = 'Running') AS ongoingGames, " //
				+ "	(SELECT SUM(gameCount) FROM games_played_stats WHERE gametype IN (0, 5)) AS totalSinglePlayerGames, " //
				+ "	(SELECT SUM(gameCount) FROM games_played_stats WHERE gametype IN (1, 2)) AS totalMultiPlayerGames";
	}

	public static String getQuerySelectMaxPlayerHallOfFame() {
		return " SELECT MAX(id) FROM hall_of_fame ";
	}

	public static String getQueryInsertHallofFame() {
		return " INSERT INTO hall_of_fame (username, nation, score, end_turn, end_date, ip) " //
				+ " VALUES (?, ?, ?, ?, NOW(), ?) ";
	}

	public static String getQueryDeleteServers() {
		return " DELETE FROM servers " //
				+ " WHERE host = ? AND port = ? ";
	}

	public static String getQueryDeleteVariables() {
		return " DELETE FROM variables " //
				+ " WHERE hostport = ? ";
	}

	public static String getQueryDeletePlayers() {
		return " DELETE FROM players " //
				+ " WHERE hostport = ? ";
	}

	public static String getQueryUpdateServer() {
		return " UPDATE servers SET available = 0, humans = -1 " //
				+ " WHERE host = ? AND port = ?";
	}

	public static String getQueryInsertPlayer() {
		return " INSERT INTO players (hostport, user, name, nation, flag, type, host) " //
				+ " VALUES (?, ?, ?, ?, ?, ?, ?) ";
	}

	public static String getQueryInsertVariable() {
		return " INSERT INTO variables (hostport, name, value) " //
				+ " VALUES (?, ?, ?) ";
	}

	public static StringBuilder getQueryUpdateServers(boolean serverExists, List<String> setServerVariables) {
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
		return queryBuilder;
	}

	public static String getQueryInsertAuthPlayer() {
		return "INSERT INTO auth (username, email, secure_hashed_password, activated, ip) " //
				+ "VALUES (?, ?, ?, ?, ?)";
	}

	public static String getQuerySelectRecentServerStats() {
		return " SELECT COUNT(*) AS count " //
				+ "	FROM servers " //
				+ " UNION ALL " //
				+ "	SELECT COUNT(*) AS count " //
				+ "	FROM servers " //
				+ "	WHERE type = 'singleplayer' " //
				+ " AND state = 'Pregame' " //
				+ " AND humans = '0' " //
				+ " AND stamp >= DATE_SUB(NOW(), INTERVAL 1 MINUTE) " //
				+ " UNION ALL " //
				+ "	SELECT COUNT(*) AS count " //
				+ "	FROM servers " //
				+ "	WHERE type = 'multiplayer' " //
				+ "	AND state = 'Pregame' " //
				+ "	AND stamp >= DATE_SUB(NOW(), INTERVAL 1 MINUTE) " //
				+ " UNION ALL " //
				+ "	SELECT COUNT(*) AS count " //
				+ "	FROM servers " //
				+ " WHERE type = 'pbem' " //
				+ " AND state = 'Pregame' " //
				+ " AND stamp >= DATE_SUB(NOW(), INTERVAL 1 MINUTE) ";
	}

	public static String getQueryUpdateAuthSecurePassword() {
		return "UPDATE auth SET secure_hashed_password = ? " //
				+ "WHERE email = ? AND activated = 1 ";
	}

	public static String getQuerySelectGoogleUser() {
		return " SELECT subject, activated, username " //
        + " FROM google_auth " //
        + " WHERE LOWER(username) = LOWER(?) OR subject = ? ";
	}

	public static String getQueryInsertGoogleUser() {
		return " INSERT INTO google_auth " //
                + " (username, subject, email, activated, ip) " //
                + " VALUES (?, ?, ?, ?, ?) ";
	}

	public static String getQueryUpdateGoogleUser() {
		return " UPDATE google_auth SET ip = ? " //
				+ " WHERE LOWER(username) = ? ";
	}
}
