<%@ page import="java.io.*,java.util.*" %>

<%
String saveGameDir = "/mnt/savegames/";
String username = "" + session.getAttribute("username");
String guest_mode = (String)request.getSession().getAttribute("guest_user");	 	
if (username != null && (guest_mode != null && guest_mode == "false")) { 

  File dir = new File(saveGameDir + username + "/");
  File[] children = dir.listFiles();

  if (children == null || children.length == 0) {
    out.println("<b>No savegames available. <a href='/wireframe.jsp?do=login'>Click here</a> to start a new game!</b>");
  } else {

    Arrays.sort( children, new Comparator()
    {
      public int compare(final Object o1, final Object o2) {
        return new Long(((File)o2).lastModified()).compareTo
             (new Long(((File) o1).lastModified()));
      }
    }); 

    for (int i=0; i<children.length; i++) {
        // Get filename of file or directory
	String filename = children[i].getName().replaceAll(".bz2", "");
	out.println("<a class='button' href='/preload.jsp?redir=/civclientlauncher?action=load&load=" + filename + "'>" 
		+ (children.length - i) + ". " + filename + "</a><br>");
    }
  }


%>


<% } %>
