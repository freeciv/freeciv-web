<%@page import="java.util.regex.*"%>
<%@page import="java.io.*"%>
<%@page import="java.util.*"%>
<%@page import="java.sql.*"%>
<%@page import="javax.servlet.*"%>
<%@page import="javax.sql.*"%>
<%@page import="javax.naming.*"%>
<%@page import="java.security.*"%>

<script>
$(document).ready(function() {
	$(".button").button();
	$("#usr").focus();
        $("#usr").val($.jStorage.get("username", ""));

	$('#usr').change(function() {
		$.jStorage.set("username", $("#usr").val());
	});

});
</script>

<div id="main_column">
<%
String redir_url = "" + request.getParameter("redir");
String username = "" + request.getParameter("username");

if (username == null || "null".equals(username)) {
%>

<div style="text-align: center;">
<center>
<h2>Choose Player Name</h2>

<div>
	<b>Please choose your player name, and click Login to begin playing:</b>
</div>
<br>

<form name="guest_user" action="/wireframe.jsp" method="get">
<input id="usr" type="text" size="15" name="username" value="" style="font-size: 20px">
<input type="hidden" name="redir" value="<%= redir_url %>">
<input type="hidden" name="do" value="guest_user">
<input class="button" type="submit" value="Login">
<br>
<br>
<br>
</form>

<br><br><br>

</div>

<% } else { 

  String PATTERN_VALIDATE_ALPHA_NUMERIC = "[0-9a-zA-Z\\.]*";
  Pattern p = Pattern.compile(PATTERN_VALIDATE_ALPHA_NUMERIC);
  if (!p.matcher(username).matches()) {
    out.println("<h2>Invalid username<h2><h3> Your username contains invalid characters. Please go back, and try again.</h3>");
  } else if(username.length() >= 32 ) {
    out.println("<h2>Invalid username</h2><h3>Your username was too long. Please go back, choose a username shorter than 32 characters and log in again.</h3>");
  } else if(username.equals("null") || username.length() <= 3 ) {
    out.println("<h2>Invalid username</h2><h3>Your username is too short. Please go back, choose a username longer than 3 characters and try again.</h3>");

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
