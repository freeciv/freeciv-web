
<%
  String act = "" + request.getParameter( "do" );
  
  if (act.equals("login")) {  
    response.sendRedirect("http://www.freeciv.net/facebook/");
    
  } else if (act.equals("dev")) { %>
   <jsp:include page="dev.jsp" flush="false"/>
  
<% } else if (act.equals("about")) { %>
  <jsp:include page="about.jsp" flush="false"/>
  
<% } else if (act.equals("browser_unsupported")) { %>
  <jsp:include page="browser_unsupported.jsp" flush="false"/>  

<% } else if (act.equals("preload")) { %>
  <jsp:include page="preload.jsp" flush="false"/>  
  
<% } else if (act.equals("menu")) { %>
  <jsp:include page="menu.jsp" flush="false"/>  
  
<% } else if (act.equals("search")) { %>
  <jsp:include page="search.jsp" flush="false"/>  
  
<% } else if (act.equals("frontpage")) { %>
  <jsp:include page="frontpage.jsp" flush="false"/>  

<% } else if (act.equals("load")) { %>
  <jsp:include page="/savegames/load.jsp" flush="false"/>
  
<% } else if (act.equals("scenarios")) { %>
  <jsp:include page="/savegames/scenarios.jsp" flush="false"/>  
 
<% } else if (act.equals("manual")) { %>
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

