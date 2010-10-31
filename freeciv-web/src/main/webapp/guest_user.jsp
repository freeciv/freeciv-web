<%@page import="java.util.regex.*"%>
<%@page import="java.io.*"%>
<%@page import="java.util.*"%>
<%@page import="java.sql.*"%>
<%@page import="javax.servlet.*"%>
<%@page import="javax.sql.*"%>
<%@page import="javax.naming.*"%>
<%@page import="java.security.*"%>
<%@page import="java.util.regex.*"%>


<div id="main_column">
<%
String redir_url = "" + request.getParameter("redir");
String username = "" + request.getParameter("username");

if (username == null || "null".equals(username)) {
%>

<div style="text-align: center;">
<center>
<h2>Choose your username:</h2>

<div>
	Please choose your guest username:
</div>

<form name="guest_user" action="/wireframe.jsp" method="get">
<input type="text" size="20" name="username" value="">
<input type="hidden" name="redir" value="<%= redir_url %>">
<input type="hidden" name="do" value="guest_user">
<input class="button" type="submit" value="Login">
</form>

<br><br><br>


<div>
	Note that you didn't <a href="/wireframe.jsp?do=openid_login">Login</a> using OpenID, 
	so this will be a guest username only.
</div>


</div>

<script>
	$( ".button").button();
</script>


<% } else { 

  String PATTERN_VALIDATE_ALPHA_NUMERIC = "[0-9a-zA-Z\\.]*";
  Pattern p = Pattern.compile(PATTERN_VALIDATE_ALPHA_NUMERIC);
  if (!p.matcher(username).matches()) {
     out.println("<h3>Invalid username, it contains invalid characters. Please go back, and try again.</h3>");
  } else if(username.equals("null") || username.length() >= 32 || username.length() <= 3 ) {
     out.println("<h3>Invalid username, it is too long or short. Please go back, and try again.</h3>");
  } else {

   Context env = (Context)(new InitialContext().lookup("java:comp/env"));
   DataSource ds = (DataSource)env.lookup("jdbc/freeciv_mysql"); 
   Connection conn = ds.getConnection();
 
   try {   
 
      PreparedStatement check_stmt = conn.prepareStatement("SELECT username FROM auth where username = ?");
      check_stmt.setString(1, username);
      ResultSet rs2 = check_stmt.executeQuery();

      PreparedStatement check_stmt2 = conn.prepareStatement("select * from players where name = ?");
      check_stmt2.setString(1, username);
      ResultSet rs3 = check_stmt2.executeQuery();

      if (rs2.next() || rs3.next()) {
        out.println("<h3>Invalid username, it is in use by an active player. Please go back, and try again.</h3>");
      } else {
        request.getSession().setAttribute( "username", username);
        request.getSession().setAttribute( "guest_user", "true");
        response.sendRedirect(redir_url);

      }
    } finally {
      conn.close();
    }       



  }


  } %>
</div>
