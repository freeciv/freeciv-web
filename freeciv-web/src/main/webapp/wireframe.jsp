<!DOCTYPE html>
<html>
<head>
<title>Freeciv.net - strategy game playable online with HTML5</title>

<link href="/stylesheets/frontpage.css" rel="stylesheet" type="text/css" />
<link type="text/css" href="/stylesheets/dark-hive-1.8.5/jquery-ui-1.8.5.custom.css" rel="stylesheet" />


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

<script type="text/javascript" src="/javascript/jquery-1.4.3.min.js"></script>
<script type="text/javascript" src="/javascript-compressed/jquery-ui-1.8.5.custom.min.js"></script>


<script type="text/javascript" src="/javascript/iphone.js"></script>
<meta name="google-site-verification" content="13_ecThQ9UAWizPUoxWp3NOhryW3hMpj7LlMAzc-og8" />


<script type="text/javascript">
/* <![CDATA[ */
    (function() {
        var s = document.createElement('script'), t = document.getElementsByTagName('script')[0];
        
        s.type = 'text/javascript';
        s.async = true;
        s.src = 'http://api.flattr.com/js/0.5.0/load.js?mode=auto';
        
        t.parentNode.insertBefore(s, t);
    })();
/* ]]> */
</script>



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
<div id="header_ads" style="text-align: center;">

<script type="text/javascript"><!--
google_ad_client = "pub-4977952202639520";
/* 728x15, opprettet 29.06.10 freeciv.net */
google_ad_slot = "7465052068";
google_ad_width = 728;
google_ad_height = 15;
//-->
</script>
<script type="text/javascript"
src="http://pagead2.googlesyndication.com/pagead/show_ads.js">
</script>

</div>

  <jsp:include page="flags.jsp" flush="false"/>


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




