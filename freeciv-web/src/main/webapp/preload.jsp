<html>
<head>

<script type="text/javascript">
function is_iphone()
{
  var agent=navigator.userAgent.toLowerCase();
  return (agent.indexOf('iphone')!=-1);
  //return true;
}

function fc_redirect() 
{
  if (is_iphone()) {
    window.location='/civclientlauncher?action=new'
  } else {
    window.location='/wireframe.jsp?do=menu'
  }
}
</script>

</head>

<body onload="fc_redirect();" text="#ffffff" bgcolor="#000000">

<center>
<h1>Preloading Freeciv.net tileset</h1>
<h3>Please wait...</h3>

<br>
<img src="/images/loading.gif" width="70" height="70">
<br>

<%
String ua = "" + request.getHeader("User-Agent");
boolean isOpera = ( ua != null && ua.indexOf( "Opera" ) != -1 );
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
