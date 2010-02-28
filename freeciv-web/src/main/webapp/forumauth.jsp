
<%@page import="java.io.*"%>
<%@page import="java.sql.*"%>
<%@page import="javax.servlet.*"%>
<%@page import="javax.sql.*"%>
<%@page import="javax.naming.*"%>
<%@page import="java.security.*"%>
<%@page import="java.math.*"%>
<%@page import="sun.misc.BASE64Encoder"%>
<%@page import="java.util.regex.*"%>

<%
  String method = request.getParameter("method");
  String rusername = request.getParameter("username");
  String susername = "" + session.getAttribute("username");

  if (method.equals("Login")) {
    if (susername.equals("andreas")) {
      out.println("{\"success\" :true, \"username\" : \"" 
              + susername+  "\", \"email\" : \"andrearo@pvv.ntnu.no\", \"isadmin\" : true}");
 
    } else if (susername != null && rusername != null &&  !"null".equals(susername)) {

       if (!susername.equals(rusername)) {
         out.println("{\"success\" :false, \"isadmin\" : false}");

       } else {
         out.println("{\"success\" :true, \"username\" : \""
             + susername + "\", \"email\" : \"-\", \"isadmin\" : false}");
       }


    } else {
      out.println("{\"success\" :false, \"isadmin\" : false}");
    }
  }
  if (method.equals("Logout")) {
    session.removeAttribute( "username" );
    session.removeAttribute( "password");
    out.println("{\"success\" :true}");
  }

  if (method.equals("GetCurrentUser")) {
    if (susername.equals("andreas")) {
      out.println("{\"success\" :true, \"username\" : \"" 
              + susername+  "\", \"email\" : \"-\", \"isadmin\" : true}");

    } else if (susername != null && !"null".equals(susername)) {
      out.println("{\"success\" :true, \"username\" : \"" 
              + susername+  "\", \"email\" : \"-\", \"isadmin\" : false}");
    } else {
      out.println("{\"success\" :false}");
    }
  }

%>
   


