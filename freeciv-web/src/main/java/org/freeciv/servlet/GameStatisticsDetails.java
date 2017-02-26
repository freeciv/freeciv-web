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
import java.util.List;
import java.util.Map;

import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.freeciv.services.Statistics;
import org.json.JSONArray;
import org.json.JSONObject;

/**
 * Lists: game type statisticts
 *
 * URL: /game/statistics/details
 */
public class GameStatisticsDetails extends HttpServlet {
	private static final long serialVersionUID = 1L;

	@Override
	public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException, ServletException {

		Statistics statistics = new Statistics();

		List<Map<String, Object>> result = statistics.getPlayedGamesByType();

		JSONArray data = new JSONArray();
		for (Map<String, Object> item : result) {
			data.put(new JSONObject(item));
		}

		try {
			request.setAttribute("data", data);
		} catch (RuntimeException e) {
			// Ohh well, we tried ...
		}

		RequestDispatcher rd = request.getRequestDispatcher("/WEB-INF/jsp/game/statistics-details.jsp");
		rd.forward(request, response);

	}

}