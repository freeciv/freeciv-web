package org.freeciv.servlet;

import java.io.IOException;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;

import javax.naming.Context;
import javax.naming.InitialContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.sql.DataSource;



/**
 * This servlet will collect statistics about time played, and number of games stated.
 */
public class FreecivStatsServlet extends HttpServlet {
	private static final long serialVersionUID = 1L;
	
	  @SuppressWarnings("unchecked")
	    public void doPost(HttpServletRequest request, HttpServletResponse response)
	            throws IOException, ServletException {

	       Connection conn = null;
	        try {

	        	String type = request.getParameter("type");
	        	if (type == null) return;
	        	
	            Context env = (Context) (new InitialContext().lookup("java:comp/env"));
	            DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
	            conn = ds.getConnection();

	            if (type.equals("time")) {
	            	String insertTableSQL = "INSERT INTO time_played_stats (statsDate, timePlayed) VALUES (CURDATE(), 1) "+
	            		"ON DUPLICATE KEY UPDATE timePlayed = timePlayed + 2";
	            	PreparedStatement preparedStatement = conn.prepareStatement(insertTableSQL);
	            	preparedStatement.executeUpdate();
	            } else if (type.equals("single")) {
	            	String insertTableSQL = "INSERT INTO games_played_stats (statsDate, gameType, gameCount) VALUES (CURDATE(), 0, 1)  ON DUPLICATE KEY UPDATE gameCount = gameCount + 1";
		            PreparedStatement preparedStatement = conn.prepareStatement(insertTableSQL);
		            preparedStatement.executeUpdate();
	            } else if (type.equals("multi")) {
	            	String insertTableSQL = "INSERT INTO games_played_stats (statsDate, gameType, gameCount) VALUES (CURDATE(), 1, 1)  ON DUPLICATE KEY UPDATE gameCount = gameCount + 1";
		            PreparedStatement preparedStatement = conn.prepareStatement(insertTableSQL);
		            preparedStatement.executeUpdate();
	            } else if (type.equals("pbem")) {
	            	String insertTableSQL = "INSERT INTO games_played_stats (statsDate, gameType, gameCount) VALUES (CURDATE(), 2, 1)  ON DUPLICATE KEY UPDATE gameCount = gameCount + 1";
		            PreparedStatement preparedStatement = conn.prepareStatement(insertTableSQL);
		            preparedStatement.executeUpdate();
		            
	            }
	            

	      } catch (Exception err) {
	            response.setHeader("result", "error");
	            response.sendError(HttpServletResponse.SC_BAD_REQUEST, "Error...");
	            return;
	        } finally {
	            if (conn != null)
	                try {
	                    conn.close();
	                } catch (SQLException e) {
	                    e.printStackTrace();
	                }
	        }


	    }

	    public void doGet(HttpServletRequest request, HttpServletResponse response)
	            throws IOException, ServletException {
	        response.getOutputStream().print("Sorry");

	    }
}
