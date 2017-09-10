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
import java.security.GeneralSecurityException;
import java.util.Collections;
import javax.servlet.*;
import javax.servlet.http.*;

import com.google.api.client.googleapis.auth.oauth2.GoogleIdToken;
import com.google.api.client.googleapis.auth.oauth2.GoogleIdToken.Payload;
import com.google.api.client.googleapis.auth.oauth2.GoogleIdTokenVerifier;
import com.google.api.client.googleapis.javanet.GoogleNetHttpTransport;
import com.google.api.client.json.jackson2.JacksonFactory;

import java.sql.*;
import javax.sql.*;
import javax.naming.*;

/**
 * Sign in with a Google Account
 * URL: /token_signin
 */
public class TokenSignin extends HttpServlet {
    private static final long serialVersionUID = 1L;

    private static final JacksonFactory jacksonFactory = new JacksonFactory();

    public void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException, ServletException {

        String idtoken = request.getParameter("idtoken");
        String username = request.getParameter("username");
        Connection conn = null;

        try {
            GoogleIdTokenVerifier verifier = new GoogleIdTokenVerifier.Builder(GoogleNetHttpTransport.newTrustedTransport(), jacksonFactory)
                    .setAudience(Collections.singletonList("122428231951-2vrvrtd9sc2v9nktemclkvc2t187jkr6.apps.googleusercontent.com"))
                    .build();
            GoogleIdToken idToken = verifier.verify(idtoken);
            if (idToken != null) {
                Payload payload = idToken.getPayload();
                // Print user identifier
                String userId = payload.getSubject();
                System.out.println("User ID: " + userId);

                Context env = (Context) (new InitialContext().lookup("java:comp/env"));
                DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
                conn = ds.getConnection();

                // 1. Check if username and userId is already stored in the database,
                //  and check if the username and userId matches.

                String authQuery =
                        "SELECT subject "
                                + "FROM google_auth "
                                + "WHERE LOWER(username) = LOWER(?) "
                                + "	AND activated = '1' LIMIT 1";
                PreparedStatement ps1 = conn.prepareStatement(authQuery);
                ps1.setString(1, username);
                ResultSet rs1 = ps1.executeQuery();
                if (!rs1.next()) {
                    // if username not found, then a new user.
                    String query = "INSERT INTO google_auth (username, subject, activated) "
                            + "VALUES (?, ?, ?)";
                    PreparedStatement preparedStatement = conn.prepareStatement(query);
                    preparedStatement.setString(1, username.toLowerCase());
                    preparedStatement.setString(2, userId);
                    preparedStatement.setInt(3, 1);
                    preparedStatement.executeUpdate();
                    response.getOutputStream().print(userId);
                } else {
                    String dbSubject = rs1.getString(1);

                    if (dbSubject != null && dbSubject.equals(userId)) {
                        // if username and userId matches, then login OK!
                        response.getOutputStream().print(userId);
                    } else {
                        // if username and userId doesn't match, then login not OK!
                        response.getOutputStream().print("Failed");
                    }
                }

            } else {
                response.getOutputStream().print("Failed");
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

        response.sendError(HttpServletResponse.SC_METHOD_NOT_ALLOWED, "This endpoint only supports the POST method.");

    }

}