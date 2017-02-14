<%@ taglib uri="http://java.sun.com/jsp/jstl/core" prefix="c" %>
<%@ taglib uri="http://java.sun.com/jsp/jstl/functions" prefix="fn" %>

<h3>Multiplayer Games</h3>

<c:if test="${fn:length(games) > 0}">
	<table class="metatable multiplayer">
		<tr class="meta_header">
			<th>Game Action:</th>
			<th>State</th>
			<th>Players</th>
			<th style="width: 45%">Message</th>
			<th>Turn:</th>
		</tr>
		<c:forEach items="${games}" var="game">
			<tr class="meta_row ${game.isProtected() ? 'private_game' : (game.state eq 'Running' ? 'running_game' : (game.players gt 0 ? 'pregame_with_players' : ''))}">
				<td>
					<c:choose>
						<c:when test="${game.state == 'Running'}">
							<a  class="btn btn-info" href="/webclient?action=multi&civserverport=${game.port}&amp;civserverhost=${game.host}&amp;multi=true">
								Play
							</a>
						</c:when>
						<c:otherwise>
							<a class="btn btn-info" href="/webclient?action=observe&amp;civserverport=${game.port}&amp;civserverhost=${game.host}&amp;multi=true">
								Observe
							</a>
						</c:otherwise>
					</c:choose>
					<a class='btn btn-info' href="/meta/game-details?host=${game.host}&amp;port=${game.port}">
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
<c:if test="${fn:length(games) == 0}">
	<h2>No servers currently listed</h2>
</c:if>

