<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>
<%@ taglib prefix="fn" uri="http://java.sun.com/jsp/jstl/functions" %> 
<%@ include file="/WEB-INF/jsp/fragments/i18n.jsp" %>
<!DOCTYPE html>
<html lang="en">
<head>
	<%@include file="/WEB-INF/jsp/fragments/head.jsp"%>
	<script src="/static/javascript/index.js"></script>
	<style>
	/* Make sure that the development tools used in freeciv are not to big */
	img.small {
		max-height: 40px;
	}
	/* 2D/3D teasers must remain within their container. */
	img.teaser {
		display: block;
		margin: auto;
		width: 100%;
	}
	.statistics { text-align: center; }

   /* Make lastest blog articles look less like a list.	*/
	ul.blog-post-summary { list-style-type: none; }
	ul.blog-post-summary > li {	margin-bottom: 5px;	}
	ul.blog-post-summary > li > a {
		color: #494A49;
		display: block;
	}
	ul.blog-post-summary > li > a:nth-of-type(1n) {	text-decoration: underline; }
	ul.blog-post-summary > li > a:nth-of-type(2n) {
		font-size: x-small;
		text-decoration: none;
	}
	/* Game launcher */          
	#game-launcher {
		width: 100%;
		margin: 0 auto;
		font-family: 'Open Sans', Helvetica;
	}
	#game-launcher .game-type {
		width: 100%;
		background: #fcf1e0;
		display: inline-table;
		top: 0;
	}
	#game-launcher .game-type:not(:last-child) { margin-right: 40px; }
	#game-launcher .header {
		color: #BE602D;
		font-family: 'Fredericka the Great', cursive;
		padding: 15px;
		margin-bottom: 0px;
		background-image: -webkit-linear-gradient(top, #fcf8e3 0, #f8efc0 100%);
		background-image: -o-linear-gradient(top, #fcf8e3 0, #f8efc0 100%);
		background-image: -webkit-gradient(linear, left top, left bottom, color-stop(0, #fcf8e3), to(#f8efc0));
		background-image: linear-gradient(to bottom, #fcf8e3 0, #f8efc0 100%);
		background-repeat: repeat-x;
		filter: progid:DXImageTransform.Microsoft.gradient(startColorstr='#fffcf8e3', endColorstr='#fff8efc0', GradientType=0);
		background-color: #fcf8e3;
		border: 1px solid transparent;
		border-radius: 4px 4px 0 0;
		border-bottom: 0;
		border-color: #f5e79e;
	}
	#game-launcher .name {
		width: 100%;
		font-size: 2em;
		display: block;
		text-align: center;
		padding: 2px 0 2px;
	}
	#game-launcher .features {
		list-style: none;
		text-align: center;               
		margin: 0;
		padding: 10px 0 0 0;                
		font-size: 0.9em;
	}
	#game-launcher .btn {
		display: inline-block;
		color: rgb(255, 255, 255);
		border: 0;
		border-radius: 5px;
		padding: 10px;
		width: 180px;
		display: block;
		font-weight: 700;
		font-size: 15px;
		text-transform: uppercase;
		margin: 20px auto 10px;
		background: #f1905b;
	}
	#game-launcher a.small { width: 100px;	}
	.multiplayer-games th:last-child { width: 120px; }
	.multiplayer-games a.label:last-child { margin-left: 3px; }
	.multiplayer-games .highlight { 
		color: green;
		font-weight: bold;
	}
</style>
</head>
<body>
	<div class="container">
		<%@include file="/WEB-INF/jsp/fragments/header.jsp"%>
		
		<div class="jumbotron">
			<div class="row">
				<img src="/static/images/frontpage-jumbotron.png" alt="" class="hidden-xs hidden-sm">
			</div>
			<div class="container-fluid">
				<div class="row top-buffer-3">
					<p class="lead">
						<fmt:message key="index-lead"/>
					</p>
				</div>
			</div>
		</div> <!-- end jumbotron -->

		<div id="game-launcher">
			<div class="row">
				<div class="col-md-4 top-buffer-3">
					<div class="game-type">
						<div class="header">
							<span class="name"><i class="fa fa-graduation-cap"></i> <fmt:message key="index-game-launcher-beginner" /></span>
						</div>
						<c:if test="${pageContext.request.locale == 'en_US'}">
							<div class="features">
								First time player?<br>
								This tutorial was made for you.<br>
								<a href="https://www.youtube.com/watch?v=K6xBi2JWyZU">Quick start video</a><br>
								<a href="https://freeciv.wikia.com/wiki/The_Art_of_Freeciv-web">Art of Freeciv-web</a>
							</div>
						</c:if>
						<a href="/webclient/?action=load&amp;load=tutorial&amp;scenario=true" class="btn"><i class="fa fa-life-ring"></i> <fmt:message key="index-game-launcher-tutorial"/></a>
					</div>

					<div class="game-type top-buffer-3">
						<div class="header">
							<span class="name"><i class="fa fa-cloud-upload"></i> <fmt:message key="index-game-launcher-load-game"/></span>
						</div>
						<a href="/webclient/?action=load" class="btn"><i class="fa fa-save"></i> <fmt:message key="index-game-launcher-load"/></a>
					</div>
				</div>
				<div class="col-md-4 top-buffer-3">
					<div class="game-type">
						<div class="header">
							<span class="name"><i class="fa fa-user"></i> <fmt:message key="index-game-launcher-singleplayer" /></span>
						</div>
						<c:if test="${pageContext.request.locale == 'en_US'}">
							<div class="features">
								Play against the Freeciv AI
							</div>
						</c:if>
						<div class="row hidden-xs hidden-sm">
							<div class="col-md-6">       
								<a id="single-button" href="/webclient/?action=new" class="btn small pull-right"><i class="fa fa-flag"></i> <fmt:message key="index-game-launcher-2d"/></a>
							</div>
							<div class="col-md-6"> 
								<a href="/webclient/?action=new&renderer=webgl" class="btn small pull-left"><i class="fa fa-cube"></i> <fmt:message key="index-game-launcher-3d"/></a>
							</div>
						</div>
						<a href="/webclient/?action=new" class="btn hidden-md hidden-lg"><i class="fa fa-flag"></i> <fmt:message key="index-game-launcher-random-map-2d"/></a>
						<a href="/webclient/?action=new&renderer=webgl" class="btn hidden-md hidden-lg"><i class="fa fa-cube"></i> <fmt:message key="index-game-launcher-random-map-3d"/></a>
						<c:if test="${pageContext.request.locale == 'en_US'}">
							<div class="features">
								Start on a scenario map, such as <br> World map, America, Italy or Japan.
							</div>
						</c:if>
						<a href="/webclient/?action=load&amp;scenario=true" class="btn"><i class="fa fa-map-o"></i> <fmt:message key="index-game-launcher-scenario"/></a>
						<c:if test="${pageContext.request.locale == 'en_US'}">
							<div class="features">
								Choose your map from a real earth map.
							</div>
						</c:if>
						<a href="/freeciv-earth/" class="btn"><i class="fa fa-globe"></i> <fmt:message key="index-game-launcher-real-earth"/></a>
					</div>
				</div>
				<div class="col-md-4 top-buffer-3">
					<div class="game-type">
						<div class="header">
							<span class="name"><i class="fa fa-users"></i> <fmt:message key="index-game-launcher-multiplayer"/></span>
						</div>
						<c:if test="${pageContext.request.locale == 'en_US'}">
							<div class="features">
								Start or join a game with multiple human or AI players.
							</div>
						</c:if>
						<a href="/game/list?v=multiplayer" class="btn"><i class="fa fa-users"></i> <fmt:message key="index-game-launcher-random-map"/></a>
						<c:if test="${pageContext.request.locale == 'en_US'}">
							<div class="features">
								Start a play-by-email game where you get an e-mail <br> when it is your turn to play.
							</div>
						</c:if>
						<a href="/webclient/?action=pbem" class="btn"><i class="fa fa-envelope"></i> <fmt:message key="index-game-launcher-play-by-email"/></a>
						<c:if test="${pageContext.request.locale == 'en_US'}">
							<div class="features">
								Play multiple human players <br> on the same computer
							</div>
						</c:if>
						<a href="/webclient/?action=hotseat" class="btn"><i class="fa fa-user-plus"></i> <fmt:message key="index-game-launcher-hotseat" /></a>
					</div>
				</div>
			</div>
		</div> <!-- end game launcher -->

		
		<div id="chrome-web-store" style="display: none;" class="alert alert-warning top-buffer-3" role="alert">
			<a href="https://chrome.google.com/webstore/detail/freeciv/ldhdjhmbapbeafmhdoobnlldhfopfcgh">
				<img src="/static/images/chrome-web-store.png" alt="">
			</a>
			<a href="https://chrome.google.com/webstore/detail/freeciv/ldhdjhmbapbeafmhdoobnlldhfopfcgh">
				<fmt:message key="index-chrome-web-store"/>
			</a>
		</div>
		<div id="google-play-store" style="display: none;" class="alert alert-warning top-buffer-3" role="alert">
			<a href="https://play.google.com/store/apps/details?id=org.freeciv.play">
				<img src="/static/images/google-play-store.png" alt="">
			</a>
			<a href="https://play.google.com/store/apps/details?id=org.freeciv.play">
				<fmt:message key="index-play-store"/>
			</a>
		</div> 
		<div id="microsoft-store" style="display: none;" class="alert alert-warning top-buffer-3" role="alert">
			<a href="http://apps.microsoft.com/windows/app/freeciv/43da4eed-42cf-4105-87a2-e03f45ade081">
				<img src="/static/images/microsoft-store.png" alt="">
			</a>
			<a href="http://apps.microsoft.com/windows/app/freeciv/43da4eed-42cf-4105-87a2-e03f45ade081">
				<fmt:message key="index-microsoft-store"/>
			</a>
		</div> <!-- end apps/browser plugins -->

		<div class="row top-buffer-1">
			<div class="col-md-12">
				<script async src="https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js"></script>
				<ins class="adsbygoogle"
					style="display:block"
					data-ad-client="ca-pub-5520523052926214"
					data-ad-slot="7043279885"
					data-ad-format="auto"></ins>
				<script>
				(adsbygoogle = window.adsbygoogle || []).push({});
				</script>
			</div>
		</div>

		<div class="row">
			<div class="col-md-6">
				<div class="panel-freeciv">
					<a href="/webclient/?action=new"><img src="/static/images/2d-teaser.png" alt="Start 2D game" class="teaser"></a>
				</div>
			</div>

			<div class="col-md-6">
				<div class="panel-freeciv">
					<a href="/webclient/?action=new&renderer=webgl"><img src="/static/images/3d-teaser.png" alt="Start 3D game" class="teaser"></a>
				</div>
			</div>
		</div> <!-- end teasers -->

		<c:if test="${pageContext.request.locale == 'en_US'}"> 
			<div id="statistics" class="row">
				<div class="col-md-12">
					<div class="panel-freeciv statistics">
						<span id="statistics-singleplayer"><b>0</b></span> <fmt:message key="index-stats-singleplayer"/> <span id="statistics-multiplayer"><b>0</b></span> <fmt:message key="index-stats-multiplayer"/><br>
						<fmt:message key="index-stats-since"/>
						<a href="/game/statistics/details"><fmt:message key="index-stats-details"/></a>			
					</div>
				</div>
			</div> <!-- end statistics -->
		</c:if>
		
		<div class="row">
			<div class="col-md-12">
				<h2><fmt:message key="index-youtube"/></h2>
			</div>
		</div>
		<div class="row">
			<div class="col-md-6">
				<div class="panel-freeciv">
					<iframe class="embed-responsive-item" width="542" height="343" src="https://www.youtube.com/embed/FDVmJXM_FNc" frameborder="0" allowfullscreen></iframe>
				</div>
			</div>
			<!--  
			<div class="col-md-6">
				<div class="panel-freeciv">
					<iframe class="embed-responsive-item" width="542" height="343" src="https://www.youtube.com/embed/7b-kitw91kg" frameborder="0" allowfullscreen></iframe>
				</div>
			</div>
			-->
		</div> <!-- end youtube -->

		<div class="row top-buffer-1">
			<div class="col-md-12 ">
				<script async src="https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js"></script>
				<ins class="adsbygoogle"
					style="display:block"
					data-ad-client="ca-pub-5520523052926214"
					data-ad-slot="7043279885"
					data-ad-format="auto"></ins>
				<script>
				(adsbygoogle = window.adsbygoogle || []).push({});
				</script>
			</div>
		</div>

		<div class="row">
			<div class="col-md-6">
				<div class="panel-freeciv">
					<h3><fmt:message key="index-multiplayer-games"/></h3>
					<c:if test="${not empty games and fn:length(games) > 0}">
						<table class="table multiplayer-games">
							<thead>
								<tr>
									<th>Game name</th>
									<th>State</th>
									<th>Turn</th>
									<th>Players</th>
									<th>Actions</th>
								</tr>
							</thead>
							<tbody>
								<c:forEach items="${games}" var="game">
									<tr class="${game.players > 0 && state == 'Pregame' ? 'highlight' : ''}">
										<td>
											${game.message}
										</td>
										<td>
											${game.state}
										</td>
										<td>
											${game.turn}
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
										<td>
											<c:choose>
												<c:when test="${game.state == 'Running'}">
													<a  class="label label-success" href="/webclient/?action=multi&civserverport=${game.port}&amp;civserverhost=${game.host}&amp;multi=true">
														Play
													</a>
												</c:when>
												<c:otherwise>
													<a class="label label-success" href="/webclient/?action=observe&amp;civserverport=${game.port}&amp;civserverhost=${game.host}&amp;multi=true">
														Observe
													</a>
												</c:otherwise>
											</c:choose>
											<a class="label label-primary" href="/game/details?host=${game.host}&amp;port=${game.port}">
												Info
											</a>
										</td>
									</tr>
								</c:forEach>		
							</tbody>
						</table>
					</c:if>
					<c:if test="${empty games or fn:length(games) == 0}">
						No servers currently listed.
					</c:if>
				</div>
			</div>

			<div class="col-md-6 container" id="best-of-play-by-email">
				<div class="panel-freeciv">
					<h3><fmt:message key="index-best-of-play-by-email"/></h3>
					<table class="table">
						<thead>
							<tr>
								<th>Rank</th>
								<th>Player</th>
								<th>Wins</th>
							</tr>
						</thead>
						<tbody id="play-by-email-list">
							<!-- 
								loaded dynamically
							-->
						</tbody>
					</table>
				</div>
			</div>
		</div> <!-- end multiplayer/best play by email -->

		<div class="row" id="latest-from-blog">
			<div class="col-md-12 container">
				<h2><fmt:message key="index-latest-blog"/></h2>
				<div class="panel-freeciv">
					<ul id="latest-from-blog-articles" class="blog-post-summary">
						<!-- 
							loaded dynamically
						-->
					</ul>
				</div>
			</div>
		</div> <!-- end blog -->

		<div class="row">
			<div class="col-md-12">
				<h2><fmt:message key="index-press"/></h2>
				<div class="well">
					<h4><i><fmt:message key="index-press-pc-gamer-title"/></i></h4>
					<i><fmt:message key="index-press-pc-gamer-content"/></i>
					<br>
					<a href="http://www.pcgamer.com/freeciv-available-in-html5-browsers-worldwide-productivity-plummets/" target="new"><img style="display: block; float: right;" src="images/pcgamer.gif" alt="PC Gamer"></a>
					<br>
				</div>
			</div>
		</div> <!-- end press -->
		
		<c:if test="${pageContext.request.locale == 'en_US'}">
			<div class="row">
				<div class="col-md-12">
					<h2><fmt:message key="index-developers"/></h2>
				</div>
			</div> 
			<div class="row">
				<div class="col-md-4">
					<div class="panel-freeciv">
						<h4><fmt:message key="index-contributing"/></h4>
						Freeciv is open source software released under the GNU General Public License.
						<a href="https://github.com/freeciv/freeciv-web"><fmt:message key="index-developers"/></a> and <a href="https://github.com/freeciv/freeciv-web/wiki/Contributing-Blender-models-for-Freeciv-WebGL"><fmt:message key="index-3d-artists"/></a> are welcome to join development.
					</div>
				</div>
				<div class="col-md-4">
					<div class="panel-freeciv">
						<h4><fmt:message key="index-stack"/></h4>
						<img class="small" src="/static/images/webgl-stack.png">WebGL
						<img class="small" src="/static/images/html5-stack.png">HTML5<br>
						<img class="small" src="/static/images/tomcat-stack.png">Tomcat
						<img class="small" src="/static/images/python-stack.png">Python<br>
						<img class="small" src="/static/images/three-stack.png">Three.js
						<img class="small" src="/static/images/blender-stack.png">Blender
					</div>
				</div>
				<div class="col-md-4">
					<div class="panel-freeciv">
						<h4><fmt:message key="index-credits"/></h4>
						<ul>
							<li>Andreas R&oslash;sdal <i class="fa fa-twitter"></i>  <a href="https://twitter.com/andreasrosdal/">@andreasrosdal</a></li>
							<li>Sveinung Kvilhaugsvik <i class="fa fa-github"></i>  <a href="https://github.com/kvilhaugsvik">@kvilhaugsvik</a></li>
							<li>Marko Lindqvist <i class="fa fa-github"></i>  <a href="https://github.com/cazfi">@cazfi</a></li>
							<li><a href="http://freeciv.wikia.com/wiki/People">Full list</a></li>
						</ul>
						
					</div>
				</div>
			</div> <!-- end developers -->
		</c:if>

		<div class="row top-buffer-1">
			<div class="col-md-12">
				<script async src="https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js"></script>
				<ins class="adsbygoogle"
					style="display:block"
					data-ad-client="ca-pub-5520523052926214"
					data-ad-slot="7043279885"
					data-ad-format="auto"></ins>
				<script>
				(adsbygoogle = window.adsbygoogle || []).push({});
				</script>
			</div>
		</div>
	
		<%@include file="/WEB-INF/jsp/fragments/footer.jsp"%>
	</div>
</body>
</html>	