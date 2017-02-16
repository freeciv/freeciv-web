<%@ taglib uri="http://java.sun.com/jsp/jstl/core" prefix="c" %>
<%@ taglib uri="http://java.sun.com/jsp/jstl/functions" prefix="fn" %>

<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="author" content="The Freeciv project">
    <meta name="description" content="Freeciv is a Free and Open Source empire-building strategy game made with HTML5 which you can play in your browser, tablet or mobile device!">
    <meta name="google-site-verification" content="Dz5U0ImteDS6QJqksSs6Nq7opQXZaHLntcSUkshCF8I" />
    <meta property="og:image" content="/images/freeciv-fp-logo-2.png" />
    
    <title>Freeciv-web - strategy game playable online with HTML5</title>

    <link rel="shortcut icon" href="/images/freeciv-shortcut-icon.png">
    <link rel="apple-touch-icon" href="/images/freeciv-splash2.png" />
    <!-- Bootstrap core CSS -->
    <link href="/css/bootstrap.min.css" rel="stylesheet">
    <link href="/css/morris.css" rel="stylesheet">
    <link href="/css/jquery-ui.min.css" rel="stylesheet" />
    <link href="/meta/css/metaserver.css" rel="stylesheet">
    
    <script src="/javascript/libs/jquery.min.js"></script>
    <script src="/javascript/libs/bootstrap.min.js"></script>
    <script src="/javascript/libs/jquery-ui.min.js"></script>
    <script src="/javascript/libs/raphael-min.js"></script>
    <script src="/javascript/libs/morris.min.js"></script>
    <script src="/meta/js/meta.js"></script>
    
    <script>
	  (function(i,s,o,g,r,a,m){i['GoogleAnalyticsObject']=r;i[r]=i[r]||function(){
	  (i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o),
	  m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m)
	  })(window,document,'script','//www.google-analytics.com/analytics.js','ga');
	
	  ga('create', 'UA-40584174-1', 'auto');
	  ga('send', 'pageview');
	</script>
</head>
<body>
	<script type="text/javascript" src="//s7.addthis.com/js/300/addthis_widget.js#pubid=ra-553240ed5ba009c1" async="async"></script>
	<script type="text/javascript" src="//d2zah9y47r7bi2.cloudfront.net/releases/current/tracker.js" data-token="ee5dba6fe2e048f79b422157b450947b"></script>

	<!-- Fixed navbar -->
	<nav class="navbar navbar-inverse navbar-fixed-top">
		<div class="container">
			<div class="navbar-header">
				<button type="button" class="navbar-toggle collapsed" data-toggle="collapse" data-target="#navbar" aria-expanded="false" aria-controls="navbar">
					<span class="sr-only">Toggle navigation</span>
					<span class="icon-bar"></span>
					<span class="icon-bar"></span>
					<span class="icon-bar"></span>
				</button>
				<a class="navbar-brand" href="/"><img id="top_logo" alt="Freeciv.org" src="/images/logo-top.png"></a> <!--Logo font is: Liberation Sans Bold Italic -->
			</div>
			<div id="navbar" class="collapse navbar-collapse">
				<ul class="nav navbar-nav">
					<li><a href="/">Home</a></li>
					<li class="active"><a id="metalink" href="/meta/metaserver">Online Games</a></li>
					<li><a href="https://www.reddit.com/r/freeciv">Forum</a></li>
					<li><a href="https://github.com/freeciv/freeciv-web">Github</a></li>
					<li><a href="http://play.freeciv.org/blog/">Blog</a></li>
					<li><a href="http://www.freeciv.org/donate.html">Donate</a></li>
					<li><a href="http://www.freeciv.org/">Freeciv.org</a></li>
				</ul>
				<form class="navbar-form navbar-right searchbox">
					<iframe src="https://duckduckgo.com/search.html?width=170&amp;site=www.freeciv.org,forum.freeciv.org,freeciv.wikia.com,play.freeciv.org&amp;prefill=Search Freeciv.org" style="overflow:hidden;margin:0;padding:0;width:228px;height:40px;" frameborder="0"></iframe>
				</form>
			</div>
		</div>
	</nav>
	
	<!-- Begin page content -->
	<div class="container">
		<div class="jumbotron text-center">
			<c:if test="${not empty port}">
				<!-- message -->
				<div class="row">
					<h2>Freeciv-web server id: ${port}</h2>
					<c:if test="${not empty message}">
							${message}
					</c:if>
				</div>
				<!-- game info -->
				<div class="row">
					<div class="center-block" style="width: 600px;">
						<c:choose>
							<c:when test="${state == 'Pregame'}">
								<div>
									<a class="button" href="/webclient/?action=multi&civserverport=${port}&amp;civserverhost=${host}">
										Join
									</a>
									You can join this game now.
								</div>
							</c:when>
							<c:otherwise>
								<div>
									<a class="button" href="/webclient/?action=multi&civserverport=${port}&amp;civserverhost=${host}">
										Join/Observe
									</a>
									You can observe this game now.
								</div>
							</c:otherwise>
						</c:choose>
						<div class="table-responsive">
							<table class="table">
								<thead>
									<tr>
										<th>Version</th>
										<th>Patches</th>
										<th>Capabilities</th>
										<th>State</th>
										<th>Ruleset</th>
										<th>Server ID</th>
									</tr>
								</thead>
								<tbody>
									<tr>
										<td>${version}</td>
										<td>${patches}</td>
										<td>${capability}</td>
										<td>${state}</td>
										<td>${ruleset}</td>
										<td>${serverid}</td>
									</tr>
								</tbody>
							</table>
						</div>
					</div>
				</div>
				<!-- game info -->
				<div class="row">
					<div class="center-block" style="width: 800px;">
						<c:if test="${fn:length(players) > 0}">
							<div class="table-responsive">
								<table class="table">
									<thead>
										<tr>
											<th class="left">Flag</th>
											<th>Leader</th>
											<th>Nation</th>
											<th>User</th>
											<th>Type</th>
										</tr>
									</thead>
									<tbody>
										<c:forEach items="${players}" var="player">
											<tr>
												<td>
													<c:if test="${player.flag ne 'none'}">
				   										<img src="/images/flags/${player.flag}-web.png" alt="${player.flag}" width="50">
				   									</c:if>
				   								</td> 
												<td>${player.name}</td>
												<td>${player.nation}</td>
												<td>${player.user}</td>
												<td>${player.type}</td>
											</tr>
										</c:forEach>
									</tbody>
								</table>
							</div>
						</c:if>
						<c:if test="${fn:length(players) == 0}">
							No players
						</c:if>
					</div>
				</div>
			
				<!-- scores -->
				<div class="row">
					<div class="center-block" style="width: 800px;">
						<c:if test="${state == 'Running'}">
							<b id='scores_heading'>Scores:</b><div id='scores'></div><br><br><b>Settings:</b><br>
							<script type='text/javascript'>show_scores(${port});</script>
						</c:if>
					</div>
				</div>
				
				<!-- variables -->
				<div class="row">
					<div class="center-block" style="width: 200px;">
						<c:if test="${fn:length(variables) > 0}">
							<div class="table-responsive">
								<table class="table">
									<thead>
									<tr>
										<th>Name</th>
										<th>Value</th>
									</tr>
									</thead>
									<tbody>
										<c:forEach items="${variables}" var="variable">
											<tr>
												<td>${variable.name}</td> <!-- flag goes here -->
												<td>${variable.value}</td>
											</tr>
										</c:forEach>
									</tbody>
								</table>
							</div>
						</c:if>
					</div>
				</div>
			</c:if>
			<c:if test="${empty port}">
				Cannot find the specified server
			</c:if>
		</div>
		
		<a class='button' href="/meta/metaserver">Return to games list</a>
		
		<!-- Site footer -->
		<footer class="footer">
			<p>&copy; The Freeciv Project 2013-<script type="text/javascript">document.write(new Date().getFullYear());</script>. 
				Freeciv-web is is free and open source software. 
				The Freeciv C server is released under the GNU General Public License, 
				while the Freeciv-web client is released under the GNU Affero General Public License.
			</p>
		</footer>
	</div> <!-- container -->
		


</body>
</html>
