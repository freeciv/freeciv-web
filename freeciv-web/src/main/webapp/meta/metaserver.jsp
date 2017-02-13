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
    <link href="/css/frontpage.css" rel="stylesheet">
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
	<div id="content" class="container">
		<div class="row span12 metaspan">
			<div class="row">
				<div class="col-lg-12">
					<script async
						src="//pagead2.googlesyndication.com/pagead/js/adsbygoogle.js"></script>
					<ins class="adsbygoogle" style="display: block"
						data-ad-client="ca-pub-5520523052926214" data-ad-slot="7043279885"
						data-ad-format="auto"> </ins>
					<script>
						(adsbygoogle = window.adsbygoogle || []).push({});
					</script>
				</div>
			</div>
			
			<div id="tabs">
				<ul>
					<li><a id="singleplr" href="#tabs-1">Single-player (${singlePlayerGames})</a></li>
					<li><a id="multiplr" href="#tabs-2">Multiplayer (${multiPlayerGames})</a></li>
					<li><a id="pbemplr" href="#tabs-3">Play-By-Email</a></li>
				</ul>
				<div id="tabs-1">
					<h2>Freeciv-web Single-player games</h2>
					<c:if test="${fn:length(singlePlayerGameList) > 0}">
						<table>
							<tr class="meta_header">
								<th>Game Action:</th>
								<th>Players</th>
								<th style="width: 45%">Message</th>
								<th>Player</th>
								<th>Flag</th>
								<th>Turn:</th>
							</tr>
							<c:forEach items="${singlePlayerGameList}" var="game">
								<tr class="meta_row ${game.isProtected() ? 'private_game' : '' }">
									<td>
										<a class="button" href="/webclient?action=observe&amp;civserverport=${game.port}&amp;civserverhost=${game.host}">
											Observe 2D
										</a>
										<a class="button" href="/webclient?renderer=webgl&amp;action=observe&amp;civserverport=${game.port}&amp;civserverhost=${game.host}">
											3D
										</a>
										<a class="button" href="/meta/game-details?host=${game.host}&amp;port=${game.port}">
											Game Info
										</a>
									</td>
									<td>
										${game.players}
									</td>
									<td style="width: 30%">
										${game.message}
									</td>
									<td>
										${game.player}
									</td>
									<td>
									    <c:if test="${game.flag ne 'none'}">
    										<img src="/images/flags/${game.flag}-web.png" alt="${game.flag}" width="50">
    									</c:if>
									</td>
									<td>
										${game.turn}
									</td>
								</tr>
							</c:forEach>
						</table>
					</c:if>
					<c:if test="${fn:length(singlePlayerGameList) == 0}">
						<h3>
							<a href='/webclient/?action=new'>
								Click here to start a new single player game!
							</a>
						</h3>
					</c:if>
					<br>
					<br>
					<br>
				</div>
				
				<div id="tabs-2">
					<h2>Freeciv-web Multiplayer games around the world</h2>
					<b>
						Freeciv-web multiplayer games where you can play against players
						online. Multiplayer games have simultaneous movement.
					</b>
					<br>
					<br>
					<c:if test="${fn:length(multiPlayerGamesList) > 0}">
						<table class='metatable multiplayer'>
							<tr class="meta_header">
								<th>Game Action:</th>
								<th>State</th>
								<th>Players</th>
								<th style="width: 45%;">Message</th>
								<th>Turn:</th>
							</tr>
							<c:forEach items="${multiPlayerGamesList}" var="game">
								<tr class="meta_row ${game.isProtected() ? 'private_game' : (game.state eq 'Running' ? 'running_game' : (game.players gt 0 ? 'pregame_with_players' : ''))}">
									<td>
										<c:choose>
											<c:when test="${game.state != 'Running'}">
												<a class="button" href="/webclient?action=multi&amp;civserverport=${game.port}&amp;civserverhost=${game.host}&amp;multi=true">
													Play
												</a>
											</c:when>
											<c:otherwise>
											<a class="button" href="/webclient?action=observe&amp;civserverport=${game.port}&amp;civserverhost=${game.host}&amp;multi=true">
												Join/Observe
											</a>
											<a  class="button" href="/webclient?renderer=webgl&amp;action=observe&amp;civserverport=${game.port}&amp;civserverhost=${game.host}&amp;multi=true">
												3D
											</a>
											</c:otherwise>
										</c:choose>
										<a class="button" href="/meta/game-details?host=${game.host}&amp;port=${game.port}">
											Info
										</a>
									</td>
									<td>
										${game.state}
									</td>
									<td>
										<c:choose>
											<c:when test="${game.players == 0}">
												None
											</c:when>
											<c:when test="${game.players == 1}">
												1 player
											</c:when>
											<c:otherwise>
												${game.players} players
											</c:otherwise>
										</c:choose>
									</td>
									<td style="width: 30%">
										${game.message}
									</td>
									<td>
										${game.turn}
									</td>
								</tr>
							</c:forEach>
						</table>
					</c:if>
					<c:if test="${fn:length(multiPlayerGamesList) == 0}">
						<h2>No servers currently listed</h2>
					</c:if>
					
					<br>
					<br>
				</div>
				
				<div id="tabs-3">
					<h2>Play-By-Email Games</h2>
					<b>
						A Play-By-Email game is a 1v1 deathmatch on a small map, with
						alternating turns, and players get an e-mail every time it is
						their turn to play. These games are often played over a long time
						period, each player has 7 days to complete their turn.
					</b>
					<br>

                   <h3>Current running games:</h3>

                   <table id="pbem_table" class='metatable pbem'>
                     <tr class='meta_header'><th>Players</th><th>Current player</th><th>Turn</th><th>Last played</th><th>Time left</th></tr>
                   </table>
                   <br>
                   <h3>Results of completed games:</h3>

                   <table id="pbem_result_table" class='metatable pbem'>
                        <tr class='meta_header'><th>Winner</th><th>Game Date:</th><th>Player 1:</th><th>Player 2:</th></tr>
                   </table>
                   <br><br>

                   To start a new Play-By-Email game, <a href="/webclient/?action=pbem">log in here</a>. To play your turn in a running Play-By-Email game,
                   click on the link in the last e-mail you got from Freeciv-web. Games are expired after 7 days if you don't play your turn. <br><br>

				</div>
			</div>
		</div>
		
		<!-- Site footer -->
		<div class="footer">
			<p>&copy; The Freeciv Project 2013-<script type="text/javascript">document.write(new Date().getFullYear());</script>. 
				Freeciv-web is is free and open source software. 
				The Freeciv C server is released under the GNU General Public License, 
				while the Freeciv-web client is released under the GNU Affero General Public License.
			</p>
		</div>
	</div>

</body>
</html>
