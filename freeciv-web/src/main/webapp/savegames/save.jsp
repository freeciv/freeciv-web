<%@page import="freeciv.servlet.*"%>
<%@page import="java.util.*"%>
<%@page import="java.text.DateFormat"%>
<%@page import="com.amazon.s3.*"%>

<%

String username = (String)session.getAttribute( "username");
List savegames = null;

boolean can_save = false;

if (username != null) {
  savegames = SavegameManager.getSavegames(username);
  if (savegames.size() <= 100) {
    can_save = true;
  }
}

%>



<% if (can_save) { %>
<div id="save_dialog">
<h3>Save game</h3>


Save game name:
<input id="save_name" type="text" value="" size="18" name="save_name"/>
<input type="button" value="Save now" onclick="save_game();"/>
        


</div>


<% } else { %>

You have saved more than 100 savegames, and can therefore not save any more at the moment.

<% } %>