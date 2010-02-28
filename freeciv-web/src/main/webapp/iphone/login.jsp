<%
  String username = request.getParameter( "username" );

  if (username != null && username != "" && username.length() > 3) {
    session.setAttribute( "username", username );
    response.sendRedirect("/preload.jsp");
  } else {
    out.println("Invalid username");
  }
%>
