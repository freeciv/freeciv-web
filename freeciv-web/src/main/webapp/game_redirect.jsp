<%@page import="java.io.*"%>
<%@page import="java.util.*"%>
<%@page import="java.sql.*"%>
<%@page import="javax.servlet.*"%>
<%@page import="javax.sql.*"%>
<%@page import="javax.naming.*"%>
<%@page import="java.security.*"%>
<%@page import="java.math.*"%>
<%@page import="sun.misc.BASE64Encoder"%>
<%@page import="java.util.regex.*"%>


<%

  String username = "" + request.getParameter("player");
  if (username != null) username = username.replaceAll("/", "");
  out.println("Redirecting " + username);

   Context env = (Context)(new InitialContext().lookup("java:comp/env"));
   DataSource ds = (DataSource)env.lookup("jdbc/freeciv_mysql"); 
   Connection conn = ds.getConnection();
   try {   
       PreparedStatement stmt = conn.prepareStatement("SELECT host, port, timepoint FROM player_game_list WHERE username = ?");
       stmt.setString(1, username);

       ResultSet rs = stmt.executeQuery();
       rs.next();
       String host = rs.getString(1);
       int port = rs.getInt(2);
       session.setAttribute( "opponent_username", username);
       session.setAttribute( "opponent_host", host);
       session.setAttribute( "opponent_port", port);
       response.sendRedirect("/freecivmetaserve/metaserver.php?server_port=" + host + ":" + port);
   } finally {
     conn.close();
   }       
%>			
