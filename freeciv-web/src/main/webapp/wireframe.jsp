<!DOCTYPE html>
<html>
<head>
<title>Freeciv.net - online multiplayer strategy game</title>

<link href="/stylesheets/frontpage.css" rel="stylesheet" type="text/css" />

<meta http-equiv="X-UA-Compatible" content="chrome=1">

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
  <link href="/stylesheets/fb_frontpage.css" rel="stylesheet" type="text/css" />
 <% } %>

<link rel="shortcut icon" href="/images/freeciv-forever-icon.png" />

<script type="text/javascript" src="/javascript/jquery-1.4.1.min.js"></script>

<script type="text/javascript" src="/javascript/iphone.js"></script>

</head>
<body>
<!-- HEADER -->
<div id="container">

<div id="header">

<div id="header_menu"> <a class="menu_link" href="/" title="News about Freeciv.net" target="_top">News</a> &nbsp;&nbsp; 
<a class="menu_link" href="/wireframe.jsp?do=login" title="Login to play Freeciv.net now">Play Now</a> &nbsp;&nbsp; 
<a class="menu_link" href="/freecivmetaserve/metaserver.php" title="Multiplayer Games">Games</a> &nbsp;&nbsp; 
<a class="menu_link" href="/wiki" title="Documentation">Documentation</a> &nbsp;&nbsp; 
<a class="menu_link" href="/forum/" title="Freeciv.net Forum">Forum</a> &nbsp;&nbsp;
<a class="menu_link" title="Contribute to the Freeciv.net development" href="/wireframe.jsp?do=dev">Development</a> &nbsp;&nbsp;
<a title="About Freeciv.net" class="menu_link" href="/wireframe.jsp?do=about">About</a></div>
</div>

<div id="body_content">
<br>

  <jsp:include page="template_call.jsp" flush="false"/>
</div>
<div id="footer">
Copyright &copy; 2008-2010 Freeciv.net
</div>

</div>


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




