<%@page import="freeciv.servlet.*"%>
<%@page import="java.util.*"%>
<%@page import="java.text.DateFormat"%>
<%@page import="com.amazon.s3.*"%>

<div id="main_column">

<h2>Load a saved game</h2>

<% 

String username = (String)session.getAttribute( "username");
List savegames = null;

if (username != null) {
  savegames = SavegameManager.getSavegames(username);
} else {
  response.sendRedirect("/");
}

if (savegames.size() == 0) {
  out.println("You don't have any saved games yet.");  
}

%>

<table border="0"> 

<% for (Iterator it = savegames.iterator(); it.hasNext(); ) {
    ListEntry entry = (ListEntry)it.next();
    String savename = entry.toString();
    String shortname = savename.replaceAll(username + "-savegame-", "").replaceAll(".sav.bz2", "").replaceAll(".sav.gz", "");
    
    String changed = DateFormat.getDateTimeInstance(
            DateFormat.MEDIUM, DateFormat.SHORT).format(entry.lastModified);
    
%>
<tr style="background-color: #222222;"><td>
<a href="/civclientlauncher?action=load&load=<%= savename %>">

<%= shortname %></a>
</td><td>
<%= changed %>
</td></tr>
<%  } %>
</table>

</div>
