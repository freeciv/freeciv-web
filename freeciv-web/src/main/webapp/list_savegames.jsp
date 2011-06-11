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
    out.println("<table style='font-weight: bold;' border='0'><tr><td>Please select game to resume playing:</td><td>Delete:</td></tr>");
    for (int i=0; i<children.length; i++) {
        // Get filename of file or directory
	String filename = children[i].getName().replaceAll(".bz2", "").replaceAll(".gz", "");
	out.println("<tr><td><a class='button load' href='/preload.jsp?redir=/civclientlauncher?action=load&load=" + filename + "' title='Load savegame'>" 
		+ (children.length - i) + ". " + filename + "</a></td><td>" 
                + "<a href='/delete.jsp?file=" + children[i].getName() 
		+ "' class='button delete' title='Delete savegame' onClick='return confirmSubmit();'>X</a></td></tr>");
    }
    out.println("</table>");
  }


%>


<% } %>
