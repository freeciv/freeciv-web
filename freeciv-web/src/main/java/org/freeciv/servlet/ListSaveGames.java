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

import java.io.*;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Properties;
import javax.servlet.*;
import javax.servlet.http.*;

import org.freeciv.services.Validation;


/**
 * Returns a list of savegames for the given user
 *
 * URL: /listsavegames
 */
public class ListSaveGames extends HttpServlet {
	private static final long serialVersionUID = 1L;

	private final Validation validation = new Validation();
	private String savegameDirectory;

	public void init(ServletConfig config) throws ServletException {
		super.init(config);
		savegameDirectory = System.getenv("FREECIV_WEB_SAVE_GAME_DIRECTORY");
	}

	public void doPost(HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {

		String username = request.getParameter("username");
		if (!validation.isValidUsername(username)) {
			response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
					"Invalid username");
			return;
		}

		try {
			File folder = new File(savegameDirectory + "/" + username);

			if (!folder.exists()) {
				response.getOutputStream().print(";");
			} else {
				File[] files = folder.listFiles();
				StringBuilder buffer = new StringBuilder();
				if (files != null) {
					Arrays.sort(files, new Comparator<File>() {
						public int compare(File f1, File f2) {
							return Long.compare(f2.lastModified(), f1.lastModified());
						}
					});

					for (File file : files) {
						if (file.isFile()) {
							buffer.append(file.getName().replaceAll(".sav.xz", ""));
							buffer.append(';');
						}
					}
				}
				response.getOutputStream().print(buffer.toString());
			}

		} catch (Exception err) {
			response.setHeader("result", "error");
			err.printStackTrace();
			response.sendError(HttpServletResponse.SC_BAD_REQUEST, "ERROR");
		}

	}

	public void doGet(HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {

		response.sendError(HttpServletResponse.SC_METHOD_NOT_ALLOWED, "This endpoint only supports the POST method.");

	}

}
