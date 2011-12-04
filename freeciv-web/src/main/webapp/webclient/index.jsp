<!DOCTYPE html>
<html>
<head>
	
<title>Freeciv.net - online multiplayer strategy game</title>

<link rel="stylesheet" type="text/css" href="/css/webclient.min.css" />

<script type="text/javascript" src="/javascript-compressed/jquery-1.7.1.min.js"></script>
<script type="text/javascript" src="/javascript-compressed/webclient.js"></script>
<script type="text/javascript" src="/webclient/session.jsp"></script>

<link rel="shortcut icon" href="/images/freeciv-shortcut-icon.png" />

<%--  iPhone setup --%>
<meta name="viewport" content="width=device-width; initial-scale=1.0; maximum-scale=1.0; user-scalable=0;" />
<meta name="apple-mobile-web-app-capable" content="yes" />
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent" />

<%--<link type="text/css" rel="stylesheet" media="only screen and (max-device-width: 480px)" href="/stylesheets/iphone.css">--%>
<%--<link type="text/css" rel="StyleSheet" href="/stylesheets/iphone.css" /> --%> 


</head>

<body onload="civclient_init();" onmousemove="mouse_moved_cb(event);" oncontextmenu="return allow_right_click;" onresize="mapview_window_resized();" onOrientationChange="orientation_changed();" onbeforeunload="send_surrender_game();">
	
<%
  
  String username = "" + session.getAttribute("username");
  if (username == null || "null".equals(username)) {
	// User isn't logged in.
	response.sendRedirect("/wireframe.jsp?do=login");
  }
%>

    <jsp:include page="pregame.jsp" flush="false"/>
    <jsp:include page="game.jsp" flush="false"/>
    
    
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
