
<%
  String act = "" + request.getParameter( "do" );
  
  if (act.equals("dev")) { %>
   <jsp:include page="dev.jsp" flush="false"/>
  
<% } else if (act.equals("openid_login")) { %>
  <jsp:include page="/auth/index.jsp" flush="false"/>
 
<%-- login and menu both point to the main menu.--%>
<% } else if (act.equals("login")) { %>
  <jsp:include page="menu.jsp" flush="false"/>
<% } else if (act.equals("menu")) { %>
  <jsp:include page="menu.jsp" flush="false"/>  
  
<% } else if (act.equals("guest_user")) { %>
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
  <jsp:include page="/savegames/scenarios.jsp" flush="false"/>  
  
<% } else if (act.equals("facebook_login")) {  
    // Facebook is the old authentication method.
    response.sendRedirect("http://www.freeciv.net/facebook/");
    
  } else if (act.equals("manual")) { %>
  <script type="text/javascript" src="/javascript/jquery-ui-1.7.2.custom.min.js"></script>
  <link type="text/css" href="/stylesheets/dark-hive/jquery-ui-1.7.2.custom.css" rel="stylesheet" />

  <jsp:include page="/manual/index.jsp" flush="false"/>  
  <script type="text/javascript">
    $('#tabs_manual').tabs();
  </script>
  <style>
    #body_content {
      width: 1300px;
    }
  </style> 

<% } else { 

response.sendRedirect("/index.html");

} %>

