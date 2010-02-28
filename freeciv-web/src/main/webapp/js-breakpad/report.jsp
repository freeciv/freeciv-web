<%@page import="java.io.*"%>
<%@page import="java.util.*"%>
<%@page import="java.sql.*"%>
<%@page import="javax.servlet.*"%>
<%@page import="javax.sql.*"%>
<%@page import="javax.naming.*"%>
<%@page import="java.security.*"%>
<%@page import="java.math.*"%>
<%@page import="sun.misc.BASE64Encoder"%>
<%@page import="java.util.regex.*"%>


<%
  Context env = (Context)(new InitialContext().lookup("java:comp/env"));
  DataSource ds = (DataSource)env.lookup("jdbc/freeciv_mysql"); 
  Connection conn = ds.getConnection();

  String msg = "" + request.getParameter("msg");
  String url = "" + request.getParameter("url");
  String linenumber = "" + request.getParameter("linenumber");
  String stacktrace = "" + request.getParameter("stacktrace");
  String ip = "" + request.getHeader("X-Forwarded-For");
  String useragent = "" + request.getHeader("User-Agent");

  if (linenumber == null || linenumber.equals("null")) linenumber = "0";

   try {   
       PreparedStatement stmt = conn.prepareStatement("INSERT INTO js_breakpad (msg,url,linenumber,stacktrace,timepoint, ip, useragent) VALUES (?,?,?,?,NOW(),?,?)");
       stmt.setString(1,msg);
       stmt.setString(2,url);
       stmt.setInt(3,new Integer(linenumber));
       stmt.setString(4,stacktrace);
       stmt.setString(5, ip);
       stmt.setString(6, useragent);
       stmt.executeUpdate();
   } finally {
     conn.close();
   }       
%>		
