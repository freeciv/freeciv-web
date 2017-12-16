package org.freeciv.servlet;

import org.apache.commons.io.FileUtils;
import javax.naming.Context;
import javax.naming.InitialContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.sql.DataSource;
import java.io.File;
import java.io.IOException;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.regex.Pattern;

/**
 * Submit game results to Hall of Fame.
 *
 * URL: /deactivate_user
 */
public class HallOfFamePost extends HttpServlet {

    private String PATTERN_VALIDATE_ALPHA_NUMERIC = "[0-9a-zA-Z \\.]*";
    private Pattern p = Pattern.compile(PATTERN_VALIDATE_ALPHA_NUMERIC);
    private static final String mapSrcImgPaths = "/var/lib/tomcat8/webapps/data/savegames/";
    private static final String mapDstImgPaths = "/var/lib/tomcat8/webapps/data/mapimgs/";

    public void doPost(HttpServletRequest request, HttpServletResponse response)
            throws IOException, ServletException {

        String username = java.net.URLDecoder.decode(request.getParameter("username"), "UTF-8");
        String nation = java.net.URLDecoder.decode(request.getParameter("nation"), "UTF-8");
        String score = java.net.URLDecoder.decode(request.getParameter("score"), "UTF-8");
        String turn = java.net.URLDecoder.decode(request.getParameter("turn"), "UTF-8");
        String port = java.net.URLDecoder.decode(request.getParameter("port"), "UTF-8");
        String ipAddress = request.getHeader("X-Real-IP");
        if (ipAddress == null) {
            ipAddress = request.getRemoteAddr();
        }

        if (username == null || username.length() <= 2) {
            response.sendError(HttpServletResponse.SC_BAD_REQUEST,
                    "Invalid username. Please try again with another username.");
            return;
        }

        if (!p.matcher(username).matches() || !p.matcher(score).matches() || !p.matcher(turn).matches()) {
            response.sendError(HttpServletResponse.SC_BAD_REQUEST,
                    "Invalid data submitted. ");
            return;
        }

        Connection conn = null;
        try {
            Thread.sleep(200);

            Context env = (Context) (new InitialContext().lookup("java:comp/env"));
            DataSource ds = (DataSource) env.lookup("jdbc/freeciv_mysql");
            conn = ds.getConnection();

            String idQuery = "select max(id) from hall_of_fame";
            PreparedStatement maxStmt = conn.prepareStatement(idQuery);
            ResultSet rs =  maxStmt.executeQuery();
            int newId = 1;
            while (rs.next()) {
                newId = rs.getInt(1) + 1;
            }

            File mapimg = new File(mapSrcImgPaths + "map-" + Integer.parseInt(port) + ".map.gif");
            if (mapimg.exists()) {
                FileUtils.moveFileToDirectory(mapimg, new File(mapDstImgPaths), true);
                File resultFile = new File(mapDstImgPaths + "map-" + Integer.parseInt(port) + ".map.gif");
                resultFile.renameTo(new File(mapDstImgPaths + newId + ".gif"));
            }

            String query = "INSERT INTO hall_of_fame (username, nation, score, end_turn, end_date, ip) "
                    + "VALUES (?, ?, ?, ?, NOW(), ?)";
            PreparedStatement preparedStatement = conn.prepareStatement(query);
            preparedStatement.setString(1, username);
            preparedStatement.setString(2, nation);
            preparedStatement.setString(3, score);
            preparedStatement.setString(4, turn);
            preparedStatement.setString(5, ipAddress);
            preparedStatement.executeUpdate();

        } catch (Exception err) {
            response.setHeader("result", "error");
            response.sendError(HttpServletResponse.SC_BAD_REQUEST, "Unable to submit to Hall of Fame: " + err);
        } finally {
            if (conn != null)
                try {
                    conn.close();
                } catch (SQLException e) {
                    e.printStackTrace();
                }
        }

    }

}
