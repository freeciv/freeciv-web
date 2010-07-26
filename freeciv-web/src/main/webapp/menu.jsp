<script type="text/javascript" 
src="http://ajax.googleapis.com/ajax/libs/chrome-frame/1/CFInstall.min.js"> </script>



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
  if (opponent != null && !"null".equals(opponent) && !"".equals(opponent)) {
%>

  <div class="main_menu_buttons" onmouseover="help_facebook();">
    <a href="/freecivmetaserve/metaserver.php?server_port=<%= host %>:<%= port %>">Join the game of <%= opponent %></a>
  </div>
<% } %>

<div class="main_menu_buttons" onmouseover="help_single();">
  <a href="/civclientlauncher?action=new">Start new single-player game</a>
</div>

<div class="main_menu_buttons" onmouseover="help_multi();">
  <a href="/freecivmetaserve/metaserver.php" >Join multiplayer game</a><br>
</div>

<div class="main_menu_buttons" onmouseover="help_scenario();">
  <a href="/wireframe.jsp?do=scenarios">Start scenario game</a>
</div>

<div class="main_menu_buttons" onmouseover="help_load();">
  <a href="/wireframe.jsp?do=load">Load saved game</a>
</div>
<br>
</center>
</div>


<div id="iewarning">
 <b>Internet Explorer detected:</b><br>
 Your web-browser is not optimal for this game. Internet Explorer doesn't support
 the HTML5 canvas element yet. It is recommended to get <a href="http://www.apple.com/safari/">Safari</a>,
 <a href='http://www.mozilla.com/'>Firefox</a> or <a href="http://www.google.com/chrome">Google Chrome</a> instead.
 If you want to continue using Internet Explorer, then it is highly recommended to install Google Chrome Frame
 for Internet Explorer.
 <br>

<div id="placeholder"></div>

</div>




<script>
 CFInstall.check({
    node: "placeholder",
    destination: "http://freeciv.net/wireframe.jsp?do=menu"
  });
</script>


<script type="text/javascript">

if (jQuery.browser.msie) {
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

function help_facebook_announced() {
  $("#menu_help").html("<b>Help:</b><br> Start a new multiplayer game against your Facebook friends, and invite them to observe or join your game.");
}


</script>


<div id="menu_help">
<b>Help:</b>
<br>
You have four ways to play Freeciv.net: Start a new single-player game, join another multiplayer game,
start a new scenario game, or load an existing game.
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


	<div id="ad_bottom">
		<script type="text/javascript"><!--
		google_ad_client = "pub-4977952202639520";
		/* 728x90, opprettet 19.12.09 for freeciv.net */
		google_ad_slot = "9174006131";
		google_ad_width = 728;
		google_ad_height = 90;
		//-->
		</script>
		<script type="text/javascript"
		src="http://pagead2.googlesyndication.com/pagead/show_ads.js">
		</script>
	</div>
