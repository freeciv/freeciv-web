<%@ page session="false" pageEncoding="UTF-8"%>
<%

  String server = request.getServerName();

 if (server.equals("localhost") 
   || server.equals("www.freeciv.net") 
   || server.equals("freeciv.net")
   || server.equals("games.freeciv.net")) { %> 

  <jsp:include page="/wireframe.jsp?do=frontpage" flush="false"/>
  
<% } else {%>

  <jsp:include page="/wireframe_i18n.jsp?do=frontpage_i18n" flush="false"/>

<% } %>

