/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/


package freeciv.launcher;

import java.util.regex.*;

import java.io.*;
import javax.servlet.*;
import javax.servlet.http.*;

import java.sql.*;

import javax.sql.*;
import javax.naming.*;



public class CivclientLauncher extends HttpServlet {
	/**
	 * Serialization UID.
	 */
	private static final long serialVersionUID = 1L;
	private int restartTimeLimit = 5000;

	@SuppressWarnings("unchecked")
	public void doGet(HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {

		ServletContext srvContext = request.getSession().getServletContext();
		HttpSession session = request.getSession();

		/*try {
			long lastAccessed = (Long) session.getAttribute("lastAccessed");
			if (lastAccessed > 0 && System.currentTimeMillis() < lastAccessed + restartTimeLimit) {
				response.sendError(
						HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
						"Recently joined a new game."
								+ " Please wait a short while before trying again.");
				return;
			}
		} catch (Exception err) {
		}

		session.setAttribute("lastAccessed", System.currentTimeMillis());*/

		
		/* Parse input parameters.. */
		
		String action = "" + request.getParameter("action");
		
		String username = "" + session.getAttribute("username");

		String civServerPort = "" + request.getParameter("civserverport");
		String civServerHost = "" + request.getParameter("civserverhost");
		
		String loadFileName = "" + request.getParameter("load");
		String scenario = "" + request.getParameter("scenario");

		Connection conn = null;
		try {
		  Context env = (Context)(new InitialContext().lookup("java:comp/env"));
		  DataSource ds = (DataSource)env.lookup("jdbc/freeciv_mysql"); 
		  conn = ds.getConnection();
		  
		  if (action != null && (action.equals("new") || action.equals("load"))) {
			  /* If user requested a new game, then get host and port for an available
			   * server from the metaserver DB, and use that one. */
			  PreparedStatement stmt = conn.prepareStatement("select host, port from servers where state = 'Pregame' and message LIKE '%Singleplayer%' order by rand() limit 1");
			  ResultSet rs = stmt.executeQuery();
			  if (rs.next()) {
				  civServerHost = rs.getString(1);
				  civServerPort = Integer.toString(rs.getInt(2));
			  } else {
				  response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
					"No servers available for creating a new game on.");
			  }
		  }
		
		  /* Validate port and host */
		  PreparedStatement stmt = conn.prepareStatement("SELECT count(*) FROM servers WHERE host = ? and port = ?");
	      stmt.setString(1, civServerHost);
	      stmt.setInt(2, Integer.parseInt(civServerPort));
	      ResultSet rs = stmt.executeQuery();
	       rs.next();
	       if (rs.getInt(1) != 1) {
	    	   response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
				"Invalid input values to civclient.");
	    	   return;
	       }
	       
	       PreparedStatement stmti = conn.prepareStatement("INSERT INTO player_game_list (username, host, port, timepoint) "
	    		   + "VALUES (?,?,?,NOW()) ON DUPLICATE KEY UPDATE host = VALUES(host), port = VALUES(port), timepoint = VALUES(timepoint)");
	        stmti.setString(1, username);
	       	stmti.setString(2, civServerHost);
	       	stmti.setInt(3, new Integer(civServerPort));
	        stmti.executeUpdate();
	       
		} catch (Exception err) {
			err.printStackTrace();
			return;
	   } finally {
		   if (conn != null)
			try {
				conn.close();
			} catch (SQLException e) {
				e.printStackTrace();
			}
	   }

		


		String errors = "";
		String PATTERN_VALIDATE_ALPHA_NUMERIC = "[0-9a-zA-Z\\.]*";

		Pattern p = Pattern.compile(PATTERN_VALIDATE_ALPHA_NUMERIC);
		if (!p.matcher(username).matches() || !p.matcher(civServerPort).matches()) {
			response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
					"Invalid input values to civclient.");
		}
		
		if (username == null || "null".equals(username)) {
			// User isn't logged in.
			response.sendRedirect("/wireframe.jsp?do=login");
			return;
		}

		session.setAttribute("civserverport", civServerPort);
		session.setAttribute("civserverhost", civServerHost);
		response.getOutputStream().print("success");

		
		if (!action.equals("load")) {
		  response.sendRedirect("/webclient/");
		} else {
		  response.sendRedirect("/webclient/?load=" + loadFileName + "&scenario=" + scenario);
		}

	}

}
