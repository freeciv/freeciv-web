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
import java.util.Collections;
import javax.servlet.*;
import javax.servlet.http.*;

import com.google.api.client.googleapis.auth.oauth2.GoogleIdToken;
import com.google.api.client.googleapis.auth.oauth2.GoogleIdToken.Payload;
import com.google.api.client.googleapis.auth.oauth2.GoogleIdTokenVerifier;
import com.google.api.client.googleapis.javanet.GoogleNetHttpTransport;
import com.google.api.client.json.jackson2.JacksonFactory;

import java.sql.*;
import java.util.Properties;
import javax.sql.*;
import javax.naming.*;

/**
 * Sign in with a Google Account
 * URL: /token_signin
 *
 * https://developers.google.com/identity/sign-in/web/backend-auth
 */
public class TokenSignin extends HttpServlet {
    private static final long serialVersionUID = 1L;

    private static final JacksonFactory jacksonFactory = new JacksonFactory();
    private String google_signin_key;

    public void init(ServletConfig config) throws ServletException {
        super.init(config);

        try {
            Properties prop = new Properties();
            prop.load(getServletContext().getResourceAsStream("/WEB-INF/config.properties"));
            google_signin_key = prop.getProperty("google-signin-client-key");
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException, ServletException {

        String idtoken = request.getParameter("idtoken");
        String username = request.getParameter("username");
        Connection conn = null;

        String ipAddress = request.getHeader("X-Real-IP");
        if (ipAddress == null) {
            ipAddress = request.getRemoteAddr();
        }

        try {
            GoogleIdTokenVerifier verifier = new GoogleIdTokenVerifier.Builder(GoogleNetHttpTransport.newTrustedTransport(), jacksonFactory)
                    .setAudience(Collections.singletonList(google_signin_key))
                    .build();
            GoogleIdToken idToken = verifier.verify(idtoken);
            if (idToken != null) {
                Payload payload = idToken.getPayload();

                String userId = payload.getSubject();
                String email = payload.getEmail();
                boolean emailVerified = Boolean.valueOf(payload.getEmailVerified());
                if (!emailVerified) {
                    response.getOutputStream().print("Email not verified");
                    return;
                }

                Context env = (Context) (new InitialContext().lookup("java:comp/env"));
                DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
                conn = ds.getConnection();

                // 1. Check if username and userId is already stored in the database,
                //  and check if the username and userId matches.
                String authQuery =
                        "SELECT subject, activated, username "
                                + "FROM google_auth "
                                + "WHERE LOWER(username) = LOWER(?) OR subject = ?";
                PreparedStatement ps1 = conn.prepareStatement(authQuery);
                ps1.setString(1, username);
                ps1.setString(2, userId);
                ResultSet rs1 = ps1.executeQuery();
                if (!rs1.next()) {
                    // if username or subject not found, then a new user.
                    String query = "INSERT INTO google_auth (username, subject, email, activated, ip) "
                            + "VALUES (?, ?, ?, ?, ?)";
                    PreparedStatement preparedStatement = conn.prepareStatement(query);
                    preparedStatement.setString(1, username.toLowerCase());
                    preparedStatement.setString(2, userId);
                    preparedStatement.setString(3, email);
                    preparedStatement.setInt(4, 1);
                    preparedStatement.setString(5, ipAddress);
                    preparedStatement.executeUpdate();
                    response.getOutputStream().print(userId);
                } else {
                    String dbSubject = rs1.getString(1);
                    int dbActivated = rs1.getInt(2);
                    String Username = rs1.getString(3);

                    if (dbSubject != null && dbSubject.equalsIgnoreCase(userId) && dbActivated == 1 && username.equalsIgnoreCase(Username)) {
                        // if username and userId matches, then login OK!
                        response.getOutputStream().print(userId);

                        String query = "UPDATE google_auth SET ip = ? WHERE LOWER(username) = ?";
                        PreparedStatement preparedStatement = conn.prepareStatement(query);
                        preparedStatement.setString(1, ipAddress);
                        preparedStatement.setString(2, username.toLowerCase());
                        preparedStatement.executeUpdate();


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