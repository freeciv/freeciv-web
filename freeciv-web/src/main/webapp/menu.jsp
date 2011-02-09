<%
  String server = request.getServerName();
  if (!server.equals("localhost") && !server.equals("games.freeciv.net") && 
    !server.equals("eu.freeciv.net") && !server.equals("us.freeciv.net")) {
      /* games.freeciv.net has DNS geo load-balancing. */
      response.sendRedirect ("http://games.freeciv.net/wireframe.jsp?do=menu");
  }
%>

<div id="main_column">

<div style="text-align: center;">
<center>
<h2>Main menu</h2>

<div>
  Please choose gameplay mode:
</div>

<%
  String opponent = "" + session.getAttribute("opponent_username");
  String host = "" + session.getAttribute("opponent_host");
  String port = "" + session.getAttribute("opponent_port");
  String guest_mode = (String)request.getSession().getAttribute("guest_user");
  if (opponent != null && !"null".equals(opponent) && !"".equals(opponent)) {
%>

  <div class="main_menu_buttons" onmouseover="help_facebook();">
    <a href="/preload.jsp?redir=/freecivmetaserve/metaserver.php?server_port=<%= host %>:<%= port %>">Join the game of <%= opponent %></a>
  </div>
<% } %>

<div onmouseover="help_single();">
  <a class='button' href="/preload.jsp?redir=/civclientlauncher?action=new">Start single-player game</a>
</div>

<div onmouseover="help_multi();">
  <a class="button" href="/preload.jsp?redir=/freecivmetaserve/metaserver.php" >Multiplayer game</a><br>
</div>

<div onmouseover="help_scenario();">
  <a class="button" href="/preload.jsp?redir=/wireframe.jsp?do=scenarios">Start scenario game</a>
</div>

<div onmouseover="help_tutorial();">
  <a class="button" href="/preload.jsp?redir=/tutorial.jsp">Start tutorial</a>
</div>


<br>
</center>
</div>


<div id="iewarning">
 <b>Internet Explorer detected:</b><br>
 Your web-browser is not optimal for this game. Internet Explorer doesn't support
 the HTML5 canvas element yet. Internet Explorer 9 will support Freeciv.net! It is recommended to get <a href="http://www.apple.com/safari/">Safari</a>,
 <a href='http://www.mozilla.com/'>Firefox</a> or <a href="http://www.google.com/chrome">Google Chrome</a> instead.
 <br>

<div id="placeholder"></div>

</div>


<script type="text/javascript">

if (jQuery.browser.msie && jQuery.browser.version < 9 ) {
  var div1 = document.getElementById('iewarning');
  div1.style.display = 'block';
}


function help_single() {
  $("#menu_help").html("<b>Help:</b><br> Start new single player game. Here you can play against a "
   + " number of artificial intelligence (AI) players.");
}

function help_multi() {
  $("#menu_help").html("<b>Help:</b><br> Join a multiplayer game against other players. "
  + " Multiplayer games requires at least 2 human players before they can begin. "
  + " Before the game has begun, it will be in a pregame state, where you have to wait until "
  + " enough players have joined. Once all players have joined, all players have to click "
  + " the start game button.");
}

function help_scenario() {
  $("#menu_help").html("<b>Help:</b><br> Start new scenario-game, which allows you to play a prepared game,"
  + " on the whole World, the British Aisles or the Iberian peninsula.");
}

function help_load() {
  $("#menu_help").html("<b>Help:</b><br> You can continue playing games that you have "
   + "played before, by clicking the <Load saved game> button.");
}

function help_facebook() {
  $("#menu_help").html("<b>Help:</b><br> Join this game created by your friend on Facebook.");
}

function help_tutorial() {
  $("#menu_help").html("<b>Help:</b><br> Play this tutorial scenario to get an introduction to Freeciv.net. This is recommended the first time you play Freeciv.net.");
}


function help_facebook_announced() {
  $("#menu_help").html("<b>Help:</b><br> Start a new multiplayer game against your Facebook friends, and invite them to observe or join your game.");
}

function help_openid_login() {
	$("#menu_help").html("<b>Help:</b><br> Login or sign up to a free account using OpenID. This will give you a permanent"
			 + " username, and access to savegames and other "
			 + " functionality reserved for registered users.");
}



</script>


<div id="menu_help">
<b>Help:</b>
<br>
You have five ways to play Freeciv.net: Start a new single-player game, join another multiplayer game,
start a new scenario game, play the tutorial or load an existing game.
Multiplayer games requires at least 2 human players before they can begin, while single-player 
games can start immediately.
<br><br>
It is recommended to play a few single-player games, before you try playing against
other human players.

<br><br>
Once you have joined a game, the game can be started once all human players
have clicked the "start game" button. 
</div>

</div>



<script>
	$( ".button").button();
	$( ".button").css("width", "220px");
	$( ".button").css("margin", "10px");
</script>

