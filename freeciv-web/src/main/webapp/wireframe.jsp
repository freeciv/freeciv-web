<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <title>Freeciv-web </title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="description" content="Freeciv is a Free and Open Source empire-building strategy game">
    <meta name="author" content="The Freeciv project">
    <meta name="apple-mobile-web-app-capable" content="yes">

    <!-- Le styles -->
    <link href="css/bootstrap.min.css" rel="stylesheet">
    <style type="text/css">
      body {
        padding-top: 60px;
        padding-bottom: 40px;
	background-image:url('/images/bg.jpg');

      }
    </style>
    <link href="css/bootstrap-responsive.min.css" rel="stylesheet">
<link rel="shortcut icon" href="images/freeciv-shortcut-icon.png" />
<link rel="apple-touch-icon" href="/images/freeciv-splash2.png" />
<link type="text/css" href="/css/jquery-ui.custom.min.css" rel="stylesheet" />
<script type="text/javascript" src="/javascript-compressed/jquery.min.js"></script>
<script type="text/javascript" src="/javascript/jquery-ui.custom.min.js"></script>

<script type="text/javascript" src="/javascript/json2.js"></script>
<script type="text/javascript" src="/javascript/jstorage.js"></script>

<script>
  (function(i,s,o,g,r,a,m){i['GoogleAnalyticsObject']=r;i[r]=i[r]||function(){
  (i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o),
  m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m)
  })(window,document,'script','//www.google-analytics.com/analytics.js','ga');

  ga('create', 'UA-40584174-1', 'freeciv.org');
  ga('send', 'pageview');
      
</script>


  </head>

  <body>

    <div class="navbar navbar-inverse navbar-fixed-top">
      <div class="navbar-inner">
        <div class="container">
          <button type="button" class="btn btn-navbar" data-toggle="collapse" data-target=".nav-collapse"
		onclick="window.location='/wireframe.jsp?do=menu';">
            <span class="icon-bar"></span>
            <span class="icon-bar"></span>
            <span class="icon-bar"></span>
          </button>
	<a href="/" style="padding: 3px; float: left;"><img src="/images/freeciv-web-logo.png" alt="The Freeciv Project"></a>
          <div class="nav-collapse collapse">
            <ul class="nav">
              <li><a href="/">Play Freeciv!</a></li>
              <li><a href="http://www.freeciv.org/wiki/">Wiki</a></li>
              <li><a href="/meta/metaserver.php">Current Games</a></li>
              <li><a href="http://forum.freeciv.org">Forum</a></li>
              <li><a href="http://github.com/freeciv/freeciv-web">Development</a></li>
              <li><a href="http://freeciv.wikia.com/wiki/Donations">Donate!</a></li>

            </ul>
           </div><!--/.nav-collapse -->
        </div>
      </div>
    </div>

    <div class="container">


<div id="header">

<div id="body_content">



  <jsp:include page="template_call.jsp" flush="false"/>




      <hr>

      <footer>
        <p>&copy; The Freeciv Project 2013</p>
      </footer>

    </div> <!-- /container -->

  
  </body>
</html>



