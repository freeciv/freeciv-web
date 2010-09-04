<html>
<head>
</head>

<body onload="fc_redirect();" text="#ffffff" bgcolor="#000000">

<center>


<canvas id="canvas" width="800" height="600" moz-opaque="true"></canvas>

<script type="text/javascript" src="/javascript-compressed/intro.js"></script>

<br><br>


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
