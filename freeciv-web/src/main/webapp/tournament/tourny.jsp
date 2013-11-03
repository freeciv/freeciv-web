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

<style>
.plr_info {
  background-color: white;
  padding: 2px;
  margin: 5px;
  font-weight: bold;
}

.player_box {
  min-height: 100px;
  width: 300px;
  background-color: #DDDDDD;
}

</style>

<div id="main_column">
<%
String redir_url = "" + request.getParameter("redir");
String username = "" + request.getParameter("username");
Context env = (Context)(new InitialContext().lookup("java:comp/env"));
DataSource ds = (DataSource)env.lookup("jdbc/freeciv_mysql"); 
Connection conn = ds.getConnection();



PreparedStatement stats_stmt = conn.prepareStatement("select state from servers where port = '5001';");
ResultSet rsx = stats_stmt.executeQuery();
String gamestate = "pregame";
while (rsx.next()) {
	gamestate = rsx.getString("STATE");
}

PreparedStatement plrs_stmt = conn.prepareStatement("select name from players where hostport LIKE '%5001';");
ResultSet rsp = plrs_stmt.executeQuery();
ArrayList<String> players = new ArrayList<String>();
while (rsp.next()) {
	players.add(rsp.getString("name"));
}

if (username == null || "null".equals(username)) {
%>

<div style="text-align: center;">
<center>
	<h2>Freeciv-Web Tournament</h2>
</center>

<br><br>

<div style="float: left; padding: 20px;">
	<img src="freeciv-tournament-logo.png">
</div>
<div style="float: left; padding: 20px; text-align: left;">
<b>	Welcome to the first Freeciv-Web tournament, 
	which will be today, November 3rd 2013. The tournament is starting NOW!
<br>If you've got any feedback about the Freeciv-Web tournament, please use the <a href="http://forum.freeciv.org/">Freeciv Forum</a>. 
<br>The expected time to complete the tournament game will be about 2 - 3 hours, so make sure you have thay much time available. 
<br>
<br><br>

This will be the rules of the Freeciv-web tournament:

<ul>
<li>The game will start once 120 players have joined the game. (minplayers = 120)</li>
<li>The winner will be the player who conquers all the other players, 
  <br>or the player with the highest score after 200 turns. (endturn = 200)</li>
<li>Turn timeout = 80 seconds  (can be increased during game)</li>
<li>Map size 15 000 tiles</li>
<li>Specials = 600</li>
<li>No AI players</li>
<li>Default Freeciv-Web 2.5 ruleset.</li>
</ul>

</b>
</div>

<br>


<% if (gamestate.equals("Running")) { %>

<div style="padding: 20px; clear: both; ">
	<b>The tournament is currently running. It is too late to join as a player. Enter your name to watch the game as an observer:</b>

	<br>
	<form name="guest_user" action="/tournament/index.jsp" method="get">
	<input id="usr" type="text" size="15" name="username" value="" style="font-size: 20px">
	<input type="hidden" name="redir" value="/preload.jsp?redir=%2Fcivclientlauncher%3Fcivserverport%3D5001%26civserverhost%3Dfreecivweb%26action%3Dobserve">
	<input type="hidden" name="do" value="guest_user">
	<input class="button" type="submit" value="Login">
	<br>
	<br>
	<br>
	</form>




<% } else { %>

<div style="padding: 20px; clear: both; ">
	<b>Please choose your player name, and click Login to join the Freeciv-Web Tournament:</b>

	<br>

	<form name="guest_user" action="/tournament/index.jsp" method="get">
	<input id="usr" type="text" size="15" name="username" value="" style="font-size: 20px">
	<input type="hidden" name="redir" value="/preload.jsp?redir=%2Fcivclientlauncher%3Fcivserverport%3D5001%26civserverhost%3Dfreecivweb">
	<input type="hidden" name="do" value="guest_user">
	<input class="button" type="submit" value="Login">
	<br>
	<br>
	<br>
	</form>

<% } %>

<h3>Current Gamestate: <%= gamestate %> </h3>
<br><center><img src="tournament-splash.png"></center>
<br>
<h3>Current players in the tournament:</h3>
<center><div class="player_box">
<% 
for (String p : players) {
  out.println("<div class='plr_info'>");
  out.println(p);
  out.println("</div>");
}

%>
</div>
</center>
<br>


<br><br>
This is the first time this tournament is run, so it is experimental and there might be some issues, <br>
which we'll learn from and improve in the next round!
<br>If you've got any feedback about the Freeciv-Web tournament, please use the <a href="http://forum.freeciv.org/">Freeciv Forum</a>
<br>No cheating and abusing of other players.
<br>The winner will be announced after the tournament. There si no prize for the winner.
<br>This is not an official Freeciv tournament, just something fun started by the Freeciv-web developer. So have a lot of fun!

<br><br>




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


<!-- AddThis Smart Layers BEGIN -->
<!-- Go to http://www.addthis.com/get/smart-layers to customize -->
<script type="text/javascript" src="//s7.addthis.com/js/300/addthis_widget.js#pubid=xa-5276a4f3127a515f"></script>
<script type="text/javascript">
  addthis.layers({
    'theme' : 'transparent',
    'share' : {
      'position' : 'left',
      'numPreferredServices' : 5
    }   
  });
</script>
<!-- AddThis Smart Layers END -->

