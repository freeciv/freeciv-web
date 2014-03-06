<!DOCTYPE html>
<html>
<head>

<meta name="viewport" content="width=device-width, initial-scale=1">

<%
String redir_url = "" + request.getParameter("redir");
String load = "" + request.getParameter("load");

String username = "" + session.getAttribute("username");
if (username == null || "null".equals(username)) {
	// User isn't logged in.
	redir_url = "/wireframe.jsp?do=guest_user&redir=" + redir_url;
}

if (load != null && !"null".equals(load)) {
  redir_url = redir_url + "&load=" + load;
}

%>
<script type="text/javascript" src="/javascript-compressed/jquery.min.js?ts=${initParam.buildTimeStamp}"></script>
<script type="text/javascript" src="/javascript-compressed/webclient.js?ts=${initParam.buildTimeStamp}"></script>

<script type="text/javascript">	
var progress = 0;

function fc_redirect_init()
{
    setTimeout("fc_redirect();", 300); 
}

function fc_redirect() 
{
  window.location='<%= redir_url %>'
}

function updateProgress()
{
  progress += 3;
  if (progress >= 100) progress = 0;
  var progressbar = $('#progressbar');
  progressbar.val(progress);
}

$( window ).load(function() {
  fc_redirect_init();
});

setInterval ( "updateProgress()", 300 );

</script>

</head>

<body text="#000000" bgcolor="#e6e6e6">

<center>


<br><br>

<img src="/images/freeciv-splash2.png">


<br><br>


<progress id="progressbar" value="0" max="100"></progress>


<br><br>
<h3>Please wait while Freeciv-web is loading...</h3>

  <img src="/tileset/freeciv-web-tileset-0.png" width="1" height="1">
  <img src="/tileset/freeciv-web-tileset-1.png" width="1" height="1">

</center>


</body>
</html>
