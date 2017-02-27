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

import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.annotation.MultipartConfig;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.freeciv.services.Games;
import org.freeciv.services.Statistics;

/**
 * Displays the multiplayer games
 *
 * URL: /game/list
 */
@MultipartConfig
public class GameList extends HttpServlet {

	private static final long serialVersionUID = 1L;

	@Override
	public void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {

		try {
			Games games = new Games();
			Statistics statistics = new Statistics();
			request.setAttribute("singlePlayerGames", games.getSinglePlayerCount());
			request.setAttribute("multiPlayerGames", games.getMultiPlayerCount());
			request.setAttribute("singlePlayerGameList", games.getSinglePlayerGames());
			request.setAttribute("multiPlayerGamesList", games.getMultiPlayerGames());
			request.setAttribute("playByEmailStatistics", statistics.getPlayByEmailWinners());
			request.setAttribute("view", request.getParameter("v"));
		} catch (RuntimeException err) {
			throw err;
			// Ohh well, we tried ...
		}

		RequestDispatcher rd = request.getRequestDispatcher("/WEB-INF/jsp/game/list.jsp");
		rd.forward(request, response);
	}

}