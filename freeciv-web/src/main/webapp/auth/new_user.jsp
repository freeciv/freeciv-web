<%@page import="java.io.*"%>
<%@page import="java.util.*"%>
<%@page import="java.sql.*"%>
<%@page import="javax.servlet.*"%>
<%@page import="javax.sql.*"%>
<%@page import="javax.naming.*"%>
<%@page import="java.security.*"%>

<%

// Check if OpenID user exists, or create a new user.

if (session.getAttribute("openid") != null) {

  String usersOpenID = "" + session.getAttribute("openid");
  String save_action = "" + request.getParameter("save");
  String requested_username = "" + request.getParameter("username");

  String page_errors = "";

  Context env = (Context)(new InitialContext().lookup("java:comp/env"));
  DataSource ds = (DataSource)env.lookup("jdbc/freeciv_mysql"); 
  Connection conn = ds.getConnection();
  boolean new_user = true;

  try {   
       PreparedStatement stmt = conn.prepareStatement("SELECT username FROM auth where openid_user = ?");
       stmt.setString(1, usersOpenID);

       ResultSet rs1 = stmt.executeQuery();
       if (rs1.next()) {
         /* An existing user has been found in the database. Success. */
         //System.out.println("An existing user has been found in the database. Success.");
	 new_user = false;
	 String username = rs1.getString(1);
         session.setAttribute( "username", username);
	 response.sendRedirect("/preload.jsp");  

       } else if (save_action.equals("1")) {
         /* A new OpenID user has submitted the new user form.*/
         //System.out.println("A new OpenID user has submitted the new user form.");
         PreparedStatement check_stmt = conn.prepareStatement("SELECT username FROM auth where username = ?");
         check_stmt.setString(1, requested_username);
         ResultSet rs2 = check_stmt.executeQuery();
	 if (rs2.next()) {
           page_errors += "<br><font color='red'>The username has already been taken.</font><br><br>";
	   System.out.println("The username has already been taken.");
	 } else if (requested_username == null || requested_username.equals("null") 
	            || requested_username.length() >= 32 || requested_username.length() <= 3) {
	   System.out.println("The username is invalid.");
           page_errors += "<br><font color='red'>The username is invalid. Please try another username.</font> <br><br>";

	 } else {
           //System.out.println("Creating new user: " + requested_username);
  	   PreparedStatement insert_stmt = conn.prepareStatement("INSERT INTO auth (username, openid_user) VALUES (?, ?) ");
           insert_stmt.setString(1, requested_username);
           insert_stmt.setString(2, usersOpenID);
	   insert_stmt.executeUpdate();
           session.setAttribute( "username", requested_username);
           response.sendRedirect("/preload.jsp");  
         }
       }
  } finally {
     conn.close();
  }       


%>

<% if (new_user) { %>
<div id="main_column">

<div class="register">
<div class="login_sector_fb">
<h2>Please register your Freeciv.net username</h2>

<%= page_errors %>

Please choose a username for Freeciv.net, which is the nickname that other players
will know you by. Please don't use any strange usernames with special characters.<br/><br />

<form action="/wireframe.jsp?do=new_user" method="post">
<table>
  <tr>
    <td>
      <label id="label_username" for="username">Username:</input>
    </td>
    <td>
      <input id="username" class="inputtext" type="text" size="20" value="" name="username"/>
    </td>
  </tr>
 </table>
<input type="hidden" name="save" value="1">
<input type="submit" class="inputsubmit" value="Register" style="margin-left: 80px">
</form>
</div>
</div>

</div>
<% } %>

<% } %>
