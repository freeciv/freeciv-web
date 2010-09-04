<% 
 String server = request.getServerName();
 if (server.equals("www.freeciv.net") || server.equals("freeciv.net")) { %>
# Allow everything.

<% } else {

%>User-agent: *
Disallow: /
<% } %>
