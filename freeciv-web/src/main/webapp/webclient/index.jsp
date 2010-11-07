<!DOCTYPE html>
<html>
<head>
	
<title>Freeciv.net - online multiplayer strategy game</title>

<script type="text/javascript" src="/javascript-compressed/webclient.js"></script>
<script type="text/javascript" src="/javascript-compressed/jquery.jplayer.min.js"></script>
<script type="text/javascript" src="/javascript-compressed/jmenu.js"></script>
<script type="text/javascript" src="/webclient/session.jsp"></script>

<link type="text/css" href="/stylesheets/dark-hive/jquery-ui-1.7.2.custom.css" rel="stylesheet" />
<link rel="stylesheet" type="text/css" href="/stylesheets/civclient.css" />
<link rel="stylesheet" type="text/css" href="/stylesheets/pregame.css" />
<link type="text/css" rel="StyleSheet" href="/stylesheets/bluecurve.css" />
<link type="text/css" rel="StyleSheet" href="/stylesheets/jplayer.blue.monday.css" />
<link type="text/css" rel="StyleSheet" href="/stylesheets/jmenu.css" />

<link rel="shortcut icon" href="/images/freeciv-forever-icon.png" />

<%--  iPhone setup --%>
<meta name="viewport" content="width=device-width; initial-scale=1.0; maximum-scale=1.0; user-scalable=0;" />
<meta name="apple-mobile-web-app-capable" content="yes" />
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent" />

<link type="text/css" rel="stylesheet" media="only screen and (max-device-width: 480px)" href="/stylesheets/iphone.css">
<%-- FIXME! remove this in prod. --%>
<%--<link type="text/css" rel="StyleSheet" href="/stylesheets/iphone.css" /> --%> 


<% 
  String cookieName = "facebook_mode";
  Cookie cookies [] = request.getCookies();
  Cookie myCookie = null;
  if (cookies != null) {
    for (int i = 0; i < cookies.length; i++) {
      if (cookies [i].getName().equals (cookieName)) {
        myCookie = cookies[i];
        break;
      }
    }
  }
  
  if (myCookie != null && "true".equals(myCookie.getValue())) {
 %>
  <link href="/stylesheets/fb_civclient.css" rel="stylesheet" type="text/css" />
 <% } %>

</head>

<body onload="civclient_init();" onmousemove="mouse_moved_cb(event);" oncontextmenu="return allow_right_click;" onresize="mapview_window_resized();" onOrientationChange="orientation_changed();" onbeforeunload="send_surrender_game();">
	
<%-- This is the jQuery jPlayer which plays HTML5 audio. --%>
<div id="jquery_jplayer"></div> 

<%
  
  String username = "" + session.getAttribute("username");
  if (username == null || "null".equals(username)) {
	// User isn't logged in.
	response.sendRedirect("/wireframe.jsp?do=login");
  }
%>

    <jsp:include page="pregame.jsp" flush="false"/>
    <jsp:include page="mapview_excanvas.jsp" flush="false"/>
    
    
<!-- Google Analytics Code -->
<script type="text/javascript">
var gaJsHost = (("https:" == document.location.protocol) ? "https://ssl." : "http://www.");
document.write(unescape("%3Cscript src='" + gaJsHost + "google-analytics.com/ga.js' type='text/javascript'%3E%3C/script%3E"));
</script>
<script type="text/javascript">
var pageTracker = _gat._getTracker("UA-5588010-1");
pageTracker._trackPageview();
</script>    

 
</body>
</html>
