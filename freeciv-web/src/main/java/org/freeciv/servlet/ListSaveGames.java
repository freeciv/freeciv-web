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
import java.util.regex.Pattern;
import javax.servlet.*;
import javax.servlet.http.*;


/**
 * Returns a list of savegames for the given user
 *
 * URL: /listsavegames
 */
public class ListSaveGames extends HttpServlet {
	private static final long serialVersionUID = 1L;
	private String PATTERN_VALIDATE_ALPHA_NUMERIC = "[0-9a-zA-Z\\.]*";
	private Pattern p = Pattern.compile(PATTERN_VALIDATE_ALPHA_NUMERIC);

	private String savegameDirectory;

	public void init(ServletConfig config) throws ServletException {
		super.init(config);

		try {
			Properties prop = new Properties();
			prop.load(getServletContext().getResourceAsStream("/WEB-INF/config.properties"));
			savegameDirectory = prop.getProperty("savegame_dir");
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public void doPost(HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {

		String username = request.getParameter("username");
		if (username != null && !p.matcher(username).matches()) {
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
				Arrays.sort(files, new Comparator<File>() {
					public int compare(File f1, File f2) {
						return Long.compare(f1.lastModified(), f2.lastModified());
					}
				});
				StringBuffer buffer = new StringBuffer();
				for (File file : files) {
					if (file.isFile()) {
						buffer.append(file.getName().replaceAll(".sav.xz", ""));
						buffer.append(';');
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