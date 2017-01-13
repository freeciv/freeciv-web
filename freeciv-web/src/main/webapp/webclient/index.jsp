<!DOCTYPE html>
<html>
<head>
<title>Freeciv-web</title>
<link rel="stylesheet" href="/css/font-awesome.min.css">
<link rel="stylesheet" type="text/css" href="/css/webclient.min.css?ts=${initParam.buildTimeStamp}" />
<meta name="description" content="Freeciv-Web is a Free and Open Source empire-building strategy game inspired by the history of human civilization.">
<% if (request.getServerName() != "localhost" ) { %>
<script type="text/javascript">window._trackJs = { token: 'ee5dba6fe2e048f79b422157b450947b' };</script>
<script type="text/javascript" src="https://cdn.trackjs.com/releases/current/tracker.js"></script>
<% } %>
<script type="text/javascript">
var ts="${initParam.buildTimeStamp}";
</script>
<script type="text/javascript" src="/javascript/libs/jquery.min.js?ts=${initParam.buildTimeStamp}"></script>
<script type="text/javascript" src="/javascript/webclient.min.js?ts=${initParam.buildTimeStamp}"></script>
<script type="text/javascript" src="/music/audio.min.js"></script>

<link rel="shortcut icon" href="/images/freeciv-shortcut-icon.png" />
<link rel="apple-touch-icon" href="/images/freeciv-splash2.png" />

<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0, minimal-ui" />
<meta name="apple-mobile-web-app-capable" content="yes" />
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent" />

<script src="https://www.google.com/recaptcha/api.js?onload=onloadCallback&render=explicit"
        async defer>
</script>
</head>

<body>
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
<jsp:include page="/javascript/webgl/shaders/terrain_fragment_shader.js" flush="false"/>
<jsp:include page="/javascript/webgl/shaders/terrain_vertex_shader.js" flush="false"/>
<jsp:include page="/javascript/webgl/shaders/darkness_fragment_shader.js" flush="false"/>
<jsp:include page="/javascript/webgl/shaders/darkness_vertex_shader.js" flush="false"/>
<jsp:include page="/javascript/webgl/shaders/labels_fragment_shader.js" flush="false"/>
<jsp:include page="/javascript/webgl/shaders/labels_vertex_shader.js" flush="false"/>
</html>
