
<%
  String act = "" + request.getParameter( "do" );
  
  if (act.equals("dev")) { %>
   <jsp:include page="dev.jsp" flush="false"/>
  
<%-- login and menu both point to the main menu.--%>
<% } else if (act.equals("login")) { 
  response.sendRedirect("/");

 } else if (act.equals("menu")) { 
  response.sendRedirect("/");
  
 } else if (act.equals("guest_user")) { %>
  <jsp:include page="guest_user.jsp" flush="false"/>

<% } else if (act.equals("new_user")) { %>
  <jsp:include page="/auth/new_user.jsp" flush="false"/>
 
<% } else if (act.equals("about")) { %>
  <jsp:include page="about.jsp" flush="false"/>
  
<% } else if (act.equals("browser_unsupported")) { %>
  <jsp:include page="browser_unsupported.jsp" flush="false"/>  

<% } else if (act.equals("preload")) { %>
  <jsp:include page="preload.jsp" flush="false"/>  
  
  
<% } else if (act.equals("search")) { %>
  <jsp:include page="search.jsp" flush="false"/>  
 
<% } else if (act.equals("screenshots")) { %>
  <jsp:include page="screenshots.jsp" flush="false"/>  
  
  <% } else if (act.equals("404")) { %>
  <jsp:include page="404.jsp" flush="false"/>  

<% } else if (act.equals("load")) { %>
  <jsp:include page="/load.jsp" flush="false"/>
  
<% } else if (act.equals("scenarios")) { %>
  <jsp:include page="/scenarios.jsp" flush="false"/>  
  
    
  <% } else { 

response.sendRedirect("/index.html");

} %>

