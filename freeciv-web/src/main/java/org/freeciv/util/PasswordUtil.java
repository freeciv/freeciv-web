/*******************************************************************************
 * Freeciv-web - the web version of Freeciv. https://www.freeciv.org/
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
package org.freeciv.util;

import org.apache.commons.codec.digest.Crypt;
import org.apache.commons.codec.digest.DigestUtils;
import org.mindrot.jbcrypt.BCrypt;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Centralized password hashing utility for Freeciv-web.
 *
 * <p>Provides secure bcrypt-based password hashing and verification while
 * maintaining backward compatibility with legacy hash formats found in the
 * {@code auth.secure_hashed_password} column:</p>
 * <ul>
 *   <li><b>bcrypt</b> — strings starting with {@code $2a$} or {@code $2b$} (60 chars)</li>
 *   <li><b>SHA-256</b> (legacy) — 64-character lowercase hex strings</li>
 *   <li><b>Unix crypt</b> (legacy) — strings starting with {@code $5$} or {@code $6$}
 *       (SHA-256/SHA-512 crypt variants)</li>
 * </ul>
 *
 * <h3>Migration strategy</h3>
 * <p>On successful verification of a legacy hash, callers should call
 * {@link #upgradeHashIfNeeded} to transparently re-hash the password with
 * bcrypt and update the database. This ensures all accounts migrate over
 * time without requiring password resets.</p>
 *
 * <p>This class mirrors the Python {@code password_utils.py} module used by
 * the freeciv-proxy component, ensuring consistent hashing across the stack.</p>
 */
public final class PasswordUtil {

	private static final Logger logger = Logger.getLogger(PasswordUtil.class.getName());

	/**
	 * bcrypt work factor: 2^12 = 4096 iterations per hash.
	 * OWASP recommends a minimum of 10; 12 is the widely accepted default
	 * balancing security and performance (~250ms on modern hardware).
	 */
	private static final int BCRYPT_WORK_FACTOR = 12;

	private PasswordUtil() {
		// Prevent instantiation.
	}

	/**
	 * Create a bcrypt hash for the given password.
	 *
	 * @param password the plaintext password or pre-hashed token to hash
	 * @return a bcrypt hash string (60 characters) suitable for database storage
	 */
	public static String hashPassword(String password) {
		return BCrypt.hashpw(password, BCrypt.gensalt(BCRYPT_WORK_FACTOR));
	}

	/**
	 * Verify a password against a stored hash.
	 *
	 * <p>Automatically detects whether the stored hash is bcrypt, SHA-256,
	 * or Unix crypt and uses the appropriate verification method.</p>
	 *
	 * @param password   the plaintext password or pre-hashed token to verify
	 * @param storedHash the hash value retrieved from the database
	 * @return {@code true} if the password matches, {@code false} otherwise
	 */
	public static boolean verifyPassword(String password, String storedHash) {
		if (password == null || storedHash == null) {
			return false;
		}

		if (isBcryptHash(storedHash)) {
			return verifyBcrypt(password, storedHash);
		} else if (isSha256Hash(storedHash)) {
			return verifySha256(password, storedHash);
		} else if (isUnixCryptHash(storedHash)) {
			return verifyUnixCrypt(password, storedHash);
		} else {
			logger.warning("Unrecognized password hash format (length=" + storedHash.length() + ")");
			return false;
		}
	}

	/**
	 * Check whether a stored hash uses a legacy format that should be
	 * upgraded to bcrypt.
	 *
	 * @param storedHash the hash value retrieved from the database
	 * @return {@code true} if the hash is not bcrypt and should be re-hashed
	 */
	public static boolean needsUpgrade(String storedHash) {
		if (storedHash == null) {
			return false;
		}
		return !isBcryptHash(storedHash);
	}

	/**
	 * Transparently upgrade a legacy password hash to bcrypt in the database.
	 *
	 * <p>This is best-effort: failures are logged but do not affect the
	 * caller's authentication result.</p>
	 *
	 * @param conn     an active database connection (the update will be committed)
	 * @param username the username whose hash is being upgraded
	 * @param password the verified password token to re-hash with bcrypt
	 */
	public static void upgradeHashIfNeeded(Connection conn, String username,
										   String password, String storedHash) {
		if (!needsUpgrade(storedHash)) {
			return;
		}
		try {
			String newHash = hashPassword(password);
			String query = "UPDATE auth SET secure_hashed_password = ? WHERE LOWER(username) = LOWER(?)";
			PreparedStatement ps = conn.prepareStatement(query);
			ps.setString(1, newHash);
			ps.setString(2, username);
			ps.executeUpdate();
			logger.info("Upgraded password hash to bcrypt for user: " + username);
		} catch (Exception e) {
			logger.log(Level.WARNING, "Failed to upgrade password hash for user: " + username, e);
		}
	}

	// ---- Format detection ----

	private static boolean isBcryptHash(String hash) {
		return hash.startsWith("$2a$") || hash.startsWith("$2b$");
	}

	private static boolean isSha256Hash(String hash) {
		return hash.length() == 64 && hash.matches("^[0-9a-f]{64}$");
	}

	private static boolean isUnixCryptHash(String hash) {
		return hash.startsWith("$5$") || hash.startsWith("$6$");
	}

	// ---- Verification methods ----

	private static boolean verifyBcrypt(String password, String storedHash) {
		try {
			return BCrypt.checkpw(password, storedHash);
		} catch (Exception e) {
			logger.log(Level.SEVERE, "bcrypt verification failed", e);
			return false;
		}
	}

	/**
	 * Verify against a legacy SHA-256 hex hash.
	 *
	 * <p><b>WARNING:</b> Plain SHA-256 without a salt is cryptographically
	 * weak for password storage. This method exists solely for backward
	 * compatibility during the migration to bcrypt.</p>
	 */
	private static boolean verifySha256(String password, String storedHash) {
		return storedHash.equals(DigestUtils.sha256Hex(password));
	}

	/**
	 * Verify against a legacy Unix crypt hash (SHA-256 or SHA-512 variant).
	 *
	 * <p>This handles hashes created by
	 * {@link org.apache.commons.codec.digest.Crypt#crypt(String)}.</p>
	 */
	private static boolean verifyUnixCrypt(String password, String storedHash) {
		try {
			return storedHash.equals(Crypt.crypt(password, storedHash));
		} catch (Exception e) {
			logger.log(Level.SEVERE, "Unix crypt verification failed", e);
			return false;
		}
	}
}
