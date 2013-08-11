
<script type="text/javascript" src="/javascript-compressed/savegames.js"></script>
<%
String username = "" + session.getAttribute("username");
if (username == null || "null".equals(username)) {
	// User isn't logged in, make user log in.
	String redir_url = "/wireframe.jsp?do=guest_user&redir=/wireframe.jsp?do=load";
        response.sendRedirect(redir_url);
}
%>


<div id="main_column">

<div style="text-align: center;">
<center>

<h2>Load Game</h2>

  

<table id="saveTable" style='font-weight: bold;' border='0'>
	<tr><td>Please select game to resume playing:</td></tr>
	
</table>


<br><br><br>
<div><i>Note: Savegames are stored using HTML5 local storage in your browser. Clearing your browser cache will also clear your savegames. Savegames are stored with your username. Please use the same username to load a game as when you saved the game.</i></div>
</div>
</div>

