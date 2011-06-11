<%@ page import="java.io.*,java.util.*" %>


<%
/* Delete a file.*/
  String saveGameDir = "/mnt/savegames/";
  String filename = (String)request.getParameter("file");	 	
  String username = "" + session.getAttribute("username");
  String guest_mode = (String)request.getSession().getAttribute("guest_user");	 	
  if (filename != null && filename.length() > 4 && guest_mode != null 
    && guest_mode != "true" && username != null && !"null".equals(username)) {


    if (!filename.startsWith("civgame") || !(filename.endsWith(".sav.gz") || filename.endsWith("sav.bz2") 
    || filename.endsWith(".sav")) || filename.length() < 5 || username.length() < 3
    || filename.contains("..") || filename.contains("/") || username.contains("/") || username.contains(".."))
    throw new IllegalArgumentException(
      "Invalid filename");

    File f = new File(saveGameDir + username + "/" + filename);

    if (!f.exists())
      throw new IllegalArgumentException(
      "Delete failed: no such file or directory: " + filename);

    if (!f.delete())
      throw new IllegalArgumentException("Delete failed");

  %>

  <%--File deleted.--%>
  <jsp:include page="/wireframe.jsp?do=load" flush="false"/>

<% } else  { %>
  File not deleted.
<% } %>

