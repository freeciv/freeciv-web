/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
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

***********************************************************************/


package org.freeciv.servlet;

import java.io.*;
import javax.servlet.*;
import javax.servlet.http.*;

import java.sql.*;

import javax.sql.*;
import javax.naming.*;


/**
 * Checks if a user with this username exists. 
 */
public class ValidateUser extends HttpServlet {
    private static final long serialVersionUID = 1L;

    @SuppressWarnings("unchecked")
    public void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException, ServletException {

       String userstring = request.getParameter("userstring");

       Connection conn = null;
        try {
            Context env = (Context) (new InitialContext().lookup("java:comp/env"));
            DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
            conn = ds.getConnection();

            String usrSQL = "SELECT username, activated from auth where lower(username) = lower(?) or lower(email) = lower(?)";
            PreparedStatement preparedStatement = conn.prepareStatement(usrSQL);
            preparedStatement.setString(1, userstring);
            preparedStatement.setString(2, userstring);
            ResultSet rs = preparedStatement.executeQuery();
            if (rs.next()) {
              String username = rs.getString(1);
              int activated = rs.getInt(2);
              if (activated == 1) {
                response.getOutputStream().print(username);
              } else {
                response.sendError(HttpServletResponse.SC_BAD_REQUEST, "Invalid user");
              }
  
            } else if (userstring != null && userstring.contains("@")) {
                response.getOutputStream().print("invitation");
            } else {
                response.getOutputStream().print("user_does_not_exist");
            }

      } catch (Exception err) {
            response.setHeader("result", "error");
            err.printStackTrace();
            response.sendError(HttpServletResponse.SC_BAD_REQUEST, "Unable to login");
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
