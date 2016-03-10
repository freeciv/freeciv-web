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

import java.util.ArrayList;
import java.util.List;

import java.io.*;
import javax.servlet.*;
import javax.servlet.http.*;

import java.sql.*;

import javax.sql.*;

import org.apache.commons.io.IOUtils;
import org.apache.http.HttpResponse;
import org.apache.http.NameValuePair;
import org.apache.http.client.HttpClient;
import org.apache.http.client.entity.UrlEncodedFormEntity;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.HttpClientBuilder;
import org.apache.http.message.BasicNameValuePair;

import javax.naming.*;


/**
 * creates a new play by email user account.
 */
public class NewPBEMUser extends HttpServlet {
    private static final long serialVersionUID = 1L;
    private String captcha_secret;
    private String captcha_url = "https://www.google.com/recaptcha/api/siteverify";

    public void init(ServletConfig config) throws ServletException {
        super.init(config);
        captcha_secret = getInitParameter("captcha-secret");
    }
    
    @SuppressWarnings("unchecked")
    public void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException, ServletException {

        String username = request.getParameter("username");
        String password = request.getParameter("password");
        String email = request.getParameter("email");
        String captcha = request.getParameter("captcha");
        
    	HttpClient client = HttpClientBuilder.create().build();
    	HttpPost post = new HttpPost(captcha_url);

    	List<NameValuePair> urlParameters = new ArrayList<NameValuePair>();
    	urlParameters.add(new BasicNameValuePair("secret", captcha_secret));
    	urlParameters.add(new BasicNameValuePair("response", captcha));
    	post.setEntity(new UrlEncodedFormEntity(urlParameters));

        if (!captcha_secret.contains("secret goes here")) {
          /* Validate captcha against google api. skip validation for localhost 
             where captcha_secret still has default value. */
          HttpResponse captcha_response = client.execute(post);
          InputStream in = captcha_response.getEntity().getContent();
          String body = IOUtils.toString(in, "UTF-8");
    	  if (!(body.contains("success") && body.contains("true"))) {
    		response.sendError(HttpServletResponse.SC_BAD_REQUEST, "Captcha failed!");
    		return;
    	  }
       }

       Connection conn = null;
        try {
            Thread.sleep(300);

            String ipAddress = request.getHeader("X-Real-IP");  
            if (ipAddress == null) {  
              ipAddress = request.getRemoteAddr();  
            }

            Context env = (Context) (new InitialContext().lookup("java:comp/env"));
            DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
            conn = ds.getConnection();

 
            String insertTableSQL = "INSERT INTO auth (username, email, password, activated, ip) VALUES (?,?, MD5(?), ?, ?)";
            PreparedStatement preparedStatement = conn.prepareStatement(insertTableSQL);
            preparedStatement.setString(1, username.toLowerCase());
            preparedStatement.setString(2, email);
            preparedStatement.setString(3, password);
            preparedStatement.setInt(4, 1);
            preparedStatement.setString(5, ipAddress);
            preparedStatement.executeUpdate();

      } catch (Exception err) {
            response.setHeader("result", "error");
            err.printStackTrace();
            response.sendError(HttpServletResponse.SC_BAD_REQUEST, "Unable to create user: " + err);
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
