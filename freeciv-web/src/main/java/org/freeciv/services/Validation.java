/*******************************************************************************
 * Freeciv-web
 * Copyright (C) 2018 The Freeciv-web project
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
package org.freeciv.services;

import java.util.regex.Pattern;

public class Validation {

	private static final Pattern usernamePattern = Pattern.compile("[a-z][a-z0-9]*");

	public boolean isValidUsername(String name) {

		if (name == null || name.length() <= 2 || name.length() >= 32) {
			return false;
		}
		name = name.toLowerCase();
		if (!usernamePattern.matcher(name).matches()) {
			return false;
		}
		if (name.equals("pbem")) {
			return false;
		}
		return true;
	}

}

