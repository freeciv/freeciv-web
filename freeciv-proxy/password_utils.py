# -*- coding: utf-8 -*-

'''**********************************************************************
    Freeciv-web - the web version of Freeciv. https://www.freeciv.org/
    Copyright (C) 2009-2015  The Freeciv-web project

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************'''

"""Password hashing utilities for the Freeciv-web proxy.

This module centralizes all password hashing operations, providing secure
bcrypt-based hashing and verification while maintaining backward compatibility
with legacy SHA-256 hashes stored in the database.

Hash format detection:
  - bcrypt:       Starts with '$2b$' or '$2a$' (60 characters).
  - SHA-256 (LEGACY): 64-character lowercase hexadecimal string.

Migration strategy:
  When a user authenticates successfully with a legacy SHA-256 hash, the
  caller should upgrade the stored hash to bcrypt using hash_password().
  This transparent upgrade ensures all accounts migrate to bcrypt over
  time without requiring password resets.

Note:
  The Java servlets (LoginUser, NewPBEMUser, etc.) also interact with the
  same `auth` table. Those components will need equivalent updates to
  fully complete the migration away from SHA-256. Until then, the proxy
  detects both formats and verifies accordingly.
"""

import hashlib
import logging
import re

import bcrypt

logger = logging.getLogger("freeciv-proxy")

# bcrypt work factor: 2^12 = 4096 iterations per hash.
# OWASP recommends a minimum of 10; 12 is the widely accepted default
# balancing security and performance (~250ms on modern hardware).
BCRYPT_WORK_FACTOR = 12

# Matches exactly 64 lowercase hex characters (a SHA-256 hex digest).
_SHA256_HEX_PATTERN = re.compile(r'^[0-9a-f]{64}$')


def hash_password(password):
    """Create a bcrypt hash for the given password.

    Args:
        password (str): The plaintext password or pre-hashed token to hash.

    Returns:
        str: A bcrypt hash string (60 characters) suitable for database storage.
    """
    return bcrypt.hashpw(
        password.encode('utf-8'),
        bcrypt.gensalt(rounds=BCRYPT_WORK_FACTOR)
    ).decode('utf-8')


def verify_password(password, stored_hash):
    """Verify a password against a stored hash.

    Automatically detects whether the stored hash is bcrypt or legacy SHA-256
    and uses the appropriate verification method.

    Args:
        password (str): The plaintext password or pre-hashed token to verify.
        stored_hash (str): The hash value retrieved from the database.

    Returns:
        bool: True if the password matches the stored hash, False otherwise.
    """
    if password is None or stored_hash is None:
        return False

    if _is_bcrypt_hash(stored_hash):
        return _verify_bcrypt(password, stored_hash)
    elif _is_sha256_hash(stored_hash):
        return _verify_sha256_legacy(password, stored_hash)
    else:
        logger.warning("Unrecognized password hash format (length=%d)", len(stored_hash))
        return False


def needs_upgrade(stored_hash):
    """Check whether a stored hash uses a legacy format that should be upgraded.

    Args:
        stored_hash (str): The hash value retrieved from the database.

    Returns:
        bool: True if the hash is legacy SHA-256 and should be re-hashed
              with bcrypt.
    """
    if stored_hash is None:
        return False
    return _is_sha256_hash(stored_hash)


def _is_bcrypt_hash(stored_hash):
    """Return True if the hash string is in bcrypt format."""
    return stored_hash.startswith('$2b$') or stored_hash.startswith('$2a$')


def _is_sha256_hash(stored_hash):
    """Return True if the hash string looks like a SHA-256 hex digest."""
    return bool(_SHA256_HEX_PATTERN.match(stored_hash))


def _verify_bcrypt(password, stored_hash):
    """Verify a password against a bcrypt hash."""
    try:
        return bcrypt.checkpw(
            password.encode('utf-8'),
            stored_hash.encode('utf-8')
        )
    except (ValueError, TypeError) as e:
        logger.error("bcrypt verification failed: %s", e)
        return False


def _verify_sha256_legacy(password, stored_hash):
    """Verify a password against a legacy SHA-256 hash.

    WARNING: Plain SHA-256 without a salt is cryptographically weak for
    password storage. This function exists solely for backward compatibility
    during the migration to bcrypt.
    """
    return hashlib.sha256(password.encode('utf-8')).hexdigest() == stored_hash
