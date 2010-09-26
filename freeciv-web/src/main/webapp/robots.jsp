<% 
 String server = request.getServerName();
 if (server.equals("www.freeciv.net") || server.equals("freeciv.net") 
 || server.equals("de.freeciv.net")
 || server.equals("es.freeciv.net")
 || server.equals("ja.freeciv.net")
 || server.equals("zh.freeciv.net")
 || server.equals("ar.freeciv.net")
 || server.equals("fr.freeciv.net")
 || server.equals("no.freeciv.net")
 || server.equals("ru.freeciv.net")
 ) { %>
# Allow everything.

<% } else {

%>User-agent: *
Disallow: /
<% } %>
