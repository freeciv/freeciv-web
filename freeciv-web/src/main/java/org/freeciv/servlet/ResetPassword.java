package org.freeciv.servlet;

import org.apache.commons.codec.digest.Crypt;
import org.apache.commons.codec.digest.DigestUtils;
import org.apache.commons.io.IOUtils;
import org.apache.commons.mail.DefaultAuthenticator;
import org.apache.commons.mail.Email;
import org.apache.commons.mail.EmailException;
import org.apache.commons.mail.SimpleEmail;
import org.apache.commons.text.RandomStringGenerator;
import org.apache.http.HttpResponse;
import org.apache.http.NameValuePair;
import org.apache.http.client.HttpClient;
import org.apache.http.client.entity.UrlEncodedFormEntity;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.HttpClientBuilder;
import org.apache.http.message.BasicNameValuePair;
import org.freeciv.util.Constants;

import javax.naming.Context;
import javax.naming.InitialContext;
import jakarta.servlet.ServletConfig;
import jakarta.servlet.ServletException;
import jakarta.servlet.http.HttpServlet;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import javax.sql.DataSource;
import java.io.IOException;
import java.io.InputStream;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

/**
 * Reset the password of the user and send the new password by email.
 *
 * URL: /reset_password
 */
public class ResetPassword extends HttpServlet {
	
    private static final long serialVersionUID = 1L;

    private String captchaSecret;

    private String emailUsername;

    private String emailPassword;

    private String emailHost;

    private String emailPort;

    private String emailSender;

    private String fcwHost;

    public void init(ServletConfig config) throws ServletException {
        super.init(config);

        try {
            Properties prop = new Properties();
            prop.load(getServletContext().getResourceAsStream("/WEB-INF/config.properties"));
            captchaSecret = prop.getProperty("captcha_secret");
            emailUsername = prop.getProperty("email_username");
            emailPassword = prop.getProperty("email_password");
            emailHost = prop.getProperty("email_host");
            emailPort = prop.getProperty("email_port");
            emailSender = prop.getProperty("email_sender");
            fcwHost = prop.getProperty("fcw_host");
        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    public void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException, ServletException {

        String email_parameter = request.getParameter("email");
        String captcha = java.net.URLDecoder.decode(request.getParameter("captcha"), "UTF-8");
        String username = "";

        if (email_parameter == null || email_parameter.length() <= 4 || email_parameter.length() > 90) {
            response.sendError(HttpServletResponse.SC_BAD_REQUEST,
                    "Invalid e-mail address. Please try again with another email.");
            return;
        }

        /* Validate captcha against google api if a key is defined */
        if (captchaSecret != null && captchaSecret.length() > 0) {
            HttpClient client = HttpClientBuilder.create().build();
            String captchaUrl = "https://www.google.com/recaptcha/api/siteverify";
            HttpPost post = new HttpPost(captchaUrl);

            List<NameValuePair> urlParameters = new ArrayList<>();
            urlParameters.add(new BasicNameValuePair("secret", captchaSecret));
            urlParameters.add(new BasicNameValuePair("response", captcha));
            post.setEntity(new UrlEncodedFormEntity(urlParameters));

            HttpResponse captchaResponse = client.execute(post);
            InputStream in = captchaResponse.getEntity().getContent();
            String body = IOUtils.toString(in, "UTF-8");
            if (!(body.contains("success") && body.contains("true"))) {
                response.sendError(HttpServletResponse.SC_BAD_REQUEST, "Captcha failed!");
                return;
            }
        }

        System.out.println("Resetting password for " + email_parameter + ".");

        /* Generate new random password. */
        RandomStringGenerator generator = new RandomStringGenerator.Builder()
                .withinRange('a', 'z').build();
        String randomPassword = generator.generate(11);
        String hashedPwd = DigestUtils.sha512Hex(randomPassword);

        /* Set new password in database. */
        Connection conn = null;
        try {
            Thread.sleep(1000);

            Context env = (Context) (new InitialContext().lookup(Constants.JNDI_CONNECTION));
            DataSource ds = (DataSource) env.lookup(Constants.JNDI_DDBBCON_MYSQL);
            conn = ds.getConnection();

            String query = "UPDATE auth SET secure_hashed_password = ? where email = ? and activated = 1";
            PreparedStatement preparedStatement = conn.prepareStatement(query);
            preparedStatement.setString(1, Crypt.crypt(hashedPwd));
            preparedStatement.setString(2, email_parameter);
            int noUpdated = preparedStatement.executeUpdate();
            if (noUpdated != 1) {
                response.sendError(HttpServletResponse.SC_BAD_REQUEST, "Unable to reset password.");
                return;
            }

            /* Fetch existing username. */
            String usernameQuery =
                    "SELECT username "
                            + "FROM auth "
                            + "WHERE email = ?";
            PreparedStatement ps1 = conn.prepareStatement(usernameQuery);
            ps1.setString(1, email_parameter);
            ResultSet rs1 = ps1.executeQuery();
            if (rs1.next()) {
                username = rs1.getString(1);
            }

        } catch (Exception err) {
            response.setHeader("result", "error");
            err.printStackTrace();
            response.sendError(HttpServletResponse.SC_BAD_REQUEST, "Unable to reset password: " + err);
            return;
        } finally {
            if (conn != null)
                try {
                    conn.close();
                } catch (SQLException e) {
                    e.printStackTrace();
                }
        }

        /* Send new password to the user. */
        try {
            if (emailPassword.equals("password")) return;
            Email email = new SimpleEmail();
            email.setHostName(emailHost == null || emailHost.length() == 0 ? "localhost" : emailHost);
            boolean emailAuth = emailUsername != null && emailUsername.length() > 0;
            int port;
            if (emailPort == null || emailPort.length() == 0) {
                port = emailAuth ? 587 : 25;
            } else {
                port = Integer.parseInt(emailPort);
            }
            email.setSmtpPort(port);
            if (emailAuth) {
                email.setAuthenticator(new DefaultAuthenticator(emailUsername, emailPassword));
                email.setSSLOnConnect(true);
            }
            email.setFrom(emailSender);
            email.setSubject("New password for Freeciv-web");
            email.setMsg("Your new password for " + fcwHost + " has been generated. \n\nUsername: " + username + " \nPassword: " + randomPassword);
            email.addTo(email_parameter);
            email.send();
        } catch (EmailException err) {
            response.sendError(HttpServletResponse.SC_BAD_REQUEST, "Unable to reset password.");
        }

    }


    public void doGet(HttpServletRequest request, HttpServletResponse response)
            throws IOException, ServletException {

        response.sendError(HttpServletResponse.SC_METHOD_NOT_ALLOWED, "This endpoint only supports the POST method.");

    }
}
