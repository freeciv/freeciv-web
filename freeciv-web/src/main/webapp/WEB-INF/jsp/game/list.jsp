<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core"%>
<%@ taglib prefix="fn" uri="http://java.sun.com/jsp/jstl/functions"%>
<%@ include file="/WEB-INF/jsp/fragments/i18n.jsp"%>
<!DOCTYPE html>
<html lang="en">
<head>
<%@include file="/WEB-INF/jsp/fragments/head.jsp"%>

<script src="//s7.addthis.com/js/300/addthis_widget.js#pubid=ra-553240ed5ba009c1" async="async"></script>
	
<script>
(function ($) {
	
	$(function () {
		displayPlayByEmailGames();
	});
	
	function displayPlayByEmailGames () {
		$.getJSON('/mailstatus', function(data) {
			if (data.length === 0) {
				$("#play-by-email-table").hide();
				return;
			}

			$($($(".nav-tabs").children()[2]).children()[0]).html("Play-By-Email (" + data.length + ")");

			data.reverse().forEach(function (game) {
				var turn = game[0];
				var phase = game[1];
				var players = game[2];
				
				var currentPlayer = game[2][phase];
				var lastPlayed = game[3];
				var timeLeft = game[4];
				
				players = players.map(function (player) {
					return player === currentPlayer
						? '<b>' + player + '</b>'
						: player;
				}).join(', ');
				
				if (players.indexOf("@") >= 0) {
					return;
				}

				if (players.length > 100) players = players.substring(0, 100) + "...";
				
				$("#play-by-email-table").append(
					'<tr>' +
						'<td>' +
							players +
						'</td>' +
						'<td class="hidden-xs">' +
							turn +
						'</td>' +
						'<td class="hidden-xs">' +
							lastPlayed +
						'</td>' +
						'<td>' +
							timeLeft + ' hours' +
						'</td>' +
					'</tr>'
				);
			});			
		}).fail(function (err) {
			$("#play-by-email-table").hide();
		});
	}
	
})($);

</script>
	
<style>
	.nav-tabs {
		margin-top: 5px;
	}
	.nav>li>a:hover {
		background-color: #796f6f
	}
	.nav-tabs>li>a {
		background-color: #ecb66a;
		text-transform: uppercase;
		color: #fff;
	    font-weight: 700;		
	}
	.nav-tabs>li.active>a {
		color: #fff;
	}
	.nav-tabs>li.active>a, .nav-tabs>li.active>a:hover, .nav-tabs>li.active>a:focus {
	    background-color: #be602d;
	    color: #fff;
	}
	.tab-pane {
		background-color: #fcf1e0;
	}
	.table {
		background-color: #fcf1e0;
	}
	.table td {
		vertical-align: middle;
	}
	.label-lg {
		font-size: 13px;
	}
	.label-lg:not(:last-child) {
		margin-right: 3px;
	}
	.private-game {
		font-style: italics;
	}
	.running-game {
		font-weight: bold;
	}
	.highlight {
		color: green;
		font-weight: bold;
	}
	.active-player {
		font-weight: bold;
	}
	#multiplayer-table td:last-child {
		width: 290px;
	}
	#singleplayer-table td:last-child {
		width: 140px;
	}
</style>
	
	
</head>
<body>
	<%@include file="/WEB-INF/jsp/fragments/header.jsp" %>
	
	<!-- Begin page content -->
	<div id="content" class="container">
		<div class="row">
			<div class="col-md-12">
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

		<div>
			<ul class="nav nav-tabs hidden-xs" role="tablist">
				<li role="presentation" class="${view == 'singleplayer' or empty view ? 'active' : ''}"><a href="#single-player-tab"
					aria-controls="single-player" role="tab" data-toggle="tab">Single-player (${singlePlayerGames})</a></li>
				<li role="presentation" class="${view == 'multiplayer' ? 'active' : ''}"><a href="#multi-player-tab"
					aria-controls="multi-player" role="tab" data-toggle="tab">Multiplayer (${multiPlayerGames})</a></li>
				<li role="presentation" class="${view == 'play-by-email' ? 'active' : ''}"><a href="#play-by-email-tab"
					aria-controls="play-by-email" role="tab" data-toggle="tab">Play-By-Email</a></li>
			</ul>
			<ul class="nav nav-tabs hidden-lg hidden-md hidden-sm" role="tablist">
				<li role="presentation" class="${view == 'singleplayer' or empty view ? 'active' : ''}"><a href="#single-player-tab"
					aria-controls="single-player" role="tab" data-toggle="tab">Single (${singlePlayerGames})</a></li>
				<li role="presentation" class="${view == 'multiplayer' ? 'active' : ''}"><a href="#multi-player-tab"
					aria-controls="multi-player" role="tab" data-toggle="tab">Multi (${multiPlayerGames})</a></li>
				<li role="presentation" class="${view == 'play-by-email' ? 'active' : ''}"><a href="#play-by-email-tab"
					aria-controls="play-by-email" role="tab" data-toggle="tab">Play-By-Email</a></li>
			</ul>

			<div class="tab-content">
				<div role="tabpanel" class="tab-pane ${view == 'singleplayer' or empty view ? 'active' : ''}" id="single-player-tab">
					<c:if test="${fn:length(singlePlayerGameList) > 0}">
						<table id="singleplayer-table" class="table">
							<tr>
								<th>Flag</th>
								<th class="hidden-xs">Map</th>
								<th>Player</th>
								<th class="hidden-xs">Game details</th>
								<th class="hidden-xs">Players</th>
								<th class="hidden-xs">Turn</th>
								<th>Action</th>
							</tr>
							<c:forEach items="${singlePlayerGameList}" var="game">
								<tr class="${game.isProtected() ? '.private-game' : '' }">
									<td>
										<c:if test="${game.flag ne 'none'}">
											<img src="/images/flags/${game.flag}-web.png" alt="${game.flag}" width="80" height="50" title="${game.turn}">
										</c:if>
									</td>
									<td class="hidden-xs">
									    <a href="/data/savegames/map-${game.port}.map.gif">
									        <img src="/data/savegames/map-${game.port}.map.gif" width="80" height="50">
									    </a>
									</td>
									<td><b>${game.player}</b></td>
									<td class="hidden-xs">${game.message}</td>
									<td class="hidden-xs">${game.players}</td>
									<td class="hidden-xs">${game.turn}</td>
									<td><a class="label label-success label-lg"
										href="/webclient?action=observe&amp;civserverport=${game.port}&amp;civserverhost=${game.host}" title="Observe">
											2D</a> <a class="label label-success label-lg"
										href="/webclient?renderer=webgl&amp;action=observe&amp;civserverport=${game.port}&amp;civserverhost=${game.host}" title="Observe">
											3D</a> <a class="label label-primary label-lg"
										href="/game/details?host=${game.host}&amp;port=${game.port}">
											Info</a>
									</td>
								</tr>
							</c:forEach>
						</table>
					</c:if>
					<c:if test="${fn:length(singlePlayerGameList) == 0}">
							<a class="label label-primary" href="/webclient/?action=new">Start</a> a new single player game! 
					</c:if>
				</div>
	
				<div role="tabpanel" class="tab-pane ${view == 'multiplayer' ? 'active' : ''}" id="multi-player-tab">
					<c:if test="${fn:length(multiPlayerGamesList) > 0}">
						<table id="multiplayer-table" class="table">
							<tr>
								<th class="hidden-xs">Players</th>
								<th>Message</th>
								<th>State</th>
								<th class="hidden-xs">Turn</th>
								<th>Action</th>
							</tr>
							<c:forEach items="${multiPlayerGamesList}" var="game">
								<tr
									class="${game.isProtected() ? 'private-game' : (game.state eq 'Running' ? 'running-game' : (game.players gt 0 ? 'highlight' : ''))}">
									<td class="hidden-xs">
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
									<td>${game.message}</td>
									<td>${game.state}</td>
									<td class="hidden-xs">${game.turn}</td>
									<td><c:choose>
											<c:when test="${game.state != 'Running'}">
												<a class="label label-success label-lg"
													href="/webclient?action=multi&amp;civserverport=${game.port}&amp;civserverhost=${game.host}&amp;multi=true">
													Play</a>
											</c:when>
											<c:otherwise>
                                                <a class="label label-success label-lg"
													href="/webclient?action=multi&amp;civserverport=${game.port}&amp;civserverhost=${game.host}&amp;multi=true">
													Play 2D</a>
												<a class="label label-success label-lg"
													href="/webclient?action=observe&amp;civserverport=${game.port}&amp;civserverhost=${game.host}&amp;multi=true">
													Observe 2D</a>
												<a class="label label-success label-lg"
													href="/webclient?renderer=webgl&amp;action=observe&amp;civserverport=${game.port}&amp;civserverhost=${game.host}&amp;multi=true">
													3D</a>
											</c:otherwise>
										</c:choose>
										<a class="label label-primary label-lg"	href="/game/details?host=${game.host}&amp;port=${game.port}">
											Info
										</a>
									</td>
								</tr>
							</c:forEach>
						</table>
					</c:if>
					<c:if test="${fn:length(multiPlayerGamesList) == 0}">
						No servers currently listed
					</c:if>
				</div>
	
				<div role="tabpanel" class="tab-pane ${view == 'play-by-email' ? 'active' : ''}" id="play-by-email-tab">
					<div class="row">
						<div class="col-md-12">
							<p>
								A Play-By-Email game is a deathmatch on a small map with up to 4 human players, playing 
								with alternating turns, and players get an e-mail every time it is
								their turn to play. These games are often played over a long time
								period, each player has 7 days to complete their turn.
							</p>
							<p>
								To start a new Play-By-Email game, 
								<u></u></ul><a href="/webclient/?action=pbem">log in here</a></u>. To play your turn
								in a running Play-By-Email game, click on the link in the last
								e-mail you got from Freeciv-web. Games are expired after 7 days if
								you don't play your turn.
							</p>
						</div>
					</div>
	
					<div class="row top-buffer-2">
						<div class="col-md-12">
							<h4>Ongoing games</h4>
							<table id="play-by-email-table" class="table">
								<tr>
									<th>Players</th>
									<th class="hidden-xs">Turn</th>
									<th class="hidden-xs">Last played</th>
									<th>Time left</th>
								</tr>
							</table>
							<p>Current player is marked in bold.</p>
						</div>
					</div>
	
					<div class="row top-buffer-2">
						<div class="col-md-12">
							<h4>Finished games</h4>
							<table class="table">
								<tr>
									<th>Winner</th>
									<th>Game Date</th>
									<th>Player 1</th>
									<th>Player 2</th>
								</tr>
								<c:forEach items="${playByEmailStatistics}" var="game">
									<tr>
										<td>${game.winner}</td>
										<td>${game.endDate}</td>
										<td>${game.playerOne} (W: ${game.winsByPlayerOne})</td>
										<td>${game.playerTwo} (W: ${game.winsByPlayerTwo})</td>
									</tr>
								</c:forEach>
							</table>
						</div>
					</div>
				</div>
			</div>

		    <div class="row">
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

		</div>

		<%@include file="/WEB-INF/jsp/fragments/footer.jsp"%>
	</div>

</body>
</html>
