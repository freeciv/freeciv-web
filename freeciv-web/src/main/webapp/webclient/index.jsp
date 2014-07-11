<!DOCTYPE html>
<html>
<head>
	
<title>Freeciv-web</title>

<link rel="stylesheet" type="text/css" href="/css/webclient.min.css?ts=${initParam.buildTimeStamp}" />

<script type="text/javascript">
var ts="${initParam.buildTimeStamp}";
</script>
<script type="text/javascript" src="/javascript-compressed/jquery.min.js?ts=${initParam.buildTimeStamp}"></script>
<script type="text/javascript" src="/javascript-compressed/webclient.js?ts=${initParam.buildTimeStamp}"></script>

<link rel="shortcut icon" href="/images/freeciv-shortcut-icon.png" />
<link rel="apple-touch-icon" href="/images/freeciv-splash2.png" />

<%--  iPhone setup --%>
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0, minimal-ui" />
<meta name="apple-mobile-web-app-capable" content="yes" />
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent" />


</head>

<body>

    <script type="text/javascript" src="//d2zah9y47r7bi2.cloudfront.net/releases/current/tracker.js" data-token="ee5dba6fe2e048f79b422157b450947b"></script>
    <jsp:include page="pregame.jsp" flush="false"/>
    <jsp:include page="game.jsp" flush="false"/>
    
<script>
  (function(i,s,o,g,r,a,m){i['GoogleAnalyticsObject']=r;i[r]=i[r]||function(){
  (i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o),
  m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m)
  })(window,document,'script','//www.google-analytics.com/analytics.js','ga');

  ga('create', 'UA-40584174-1', 'auto');
  ga('send', 'pageview');
</script> 
</body>
</html>
