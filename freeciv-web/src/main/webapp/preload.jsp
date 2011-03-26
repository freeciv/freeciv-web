<!DOCTYPE html>
<html>
<head>

<%
String redir_url = "" + request.getParameter("redir");

String username = "" + session.getAttribute("username");
if (username == null || "null".equals(username)) {
	// User isn't logged in.
	redir_url = "/wireframe.jsp?do=guest_user&redir=" + redir_url;
}

String ua = "" + request.getHeader("User-Agent");
boolean isOpera = ( ua != null && ua.indexOf( "Opera" ) != -1 );

%>
<script type="text/javascript" src="/javascript-compressed/jquery-1.5.1.min.js"></script>
<script type="text/javascript" src="/javascript-compressed/webclient.js"></script>

<script type="text/javascript">	
var progress = 0;

function fc_redirect_init()
{
    setTimeout("fc_redirect();", 800); 
}

function fc_redirect() 
{
  window.location='<%= redir_url %>'
}

function updateProgress()
{
  var x = 120 - progress;
  <% if (isOpera) { %>
    progress += 2;
  <% } else { %>
    progress += 6;
  <% } %>

  if (progress >= 120) progress = 0;
  document.getElementById("progress").style.backgroundPosition = "-" + x +"px 0pt";
}

window.onload=fc_redirect_init;

setInterval ( "updateProgress()", 500 );


</script>

<style type="text/css">
img.percentImage {
 background: #202020 url(/images/percentImage_back.png) top left no-repeat;
 padding: 0;
 margin: 5px 0 0 0;
 background-position: 1px 0;
}
</style>
</head>

<body onload="fc_redirect_init();" text="#eeeeee" bgcolor="#202020">

<center>


	<br><br>

 	<img src="/images/freeciv-splash.png">


	<br><br>


<img id="progress" src="/images/percentImage.png" class="percentImage" style="background-position: -120px 0pt;" />

<br><br>
<h2>Please wait while Freeciv.net is loading...</h2>

<%

if (!isOpera) {
%>
  <img src="/tileset/freeciv-web-tileset-1.png" width="1" height="1">
  <img src="/tileset/freeciv-web-tileset-2.png" width="1" height="1">

<% } else { %>
    <jsp:include page="tiles/freeciv-web-tileset-small-preload.html" flush="false"/>
<% } %>
</center>


</body>
</html>
