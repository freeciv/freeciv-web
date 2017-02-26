<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>
<%@ taglib prefix="fn" uri="http://java.sun.com/jsp/jstl/functions" %> 
<%@ include file="/WEB-INF/jsp/fragments/i18n.jsp" %>
<!DOCTYPE html>
<html lang="en">
<head>
	<%@include file="/WEB-INF/jsp/fragments/head.jsp"%>
	<script type="text/javascript" src="https://s7.addthis.com/js/300/addthis_widget.js#pubid=ra-553240ed5ba009c1" async="async"></script>
	<style>
		.table th {
			text-align: center;
		}
	</style>
</head>
<body>
	<%@include file="/WEB-INF/jsp/fragments/header.jsp"%>
	
	<!-- Begin page content -->
	<div class="container">
		<div class="text-center">
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
									<a class="label label-success" href="/webclient/?action=multi&civserverport=${port}&amp;civserverhost=${host}">
										Join</a> You can join this game now.
								</div>
							</c:when>
							<c:otherwise>
								<div>
									<a class="label label-primary" href="/webclient/?action=multi&civserverport=${port}&amp;civserverhost=${host}">
										Join/Observe</a> You can observe this game now.
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
											<th>Flag</th>
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
					<div class="center-block" style="width: 200px;">
						<c:if test="${state == 'Running'}">
							<b id='scores_heading'>Scores:</b><div id='scores'></div><br><br><b>Settings:</b><br>
							<script type='text/javascript'>show_scores(${game.port});</script>
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
			<a class="btn btn-default" href="/game/list">Return to game list</a>
		</div>
		
		
		
		<!-- Site footer -->
		<%@include file="/WEB-INF/jsp/fragments/footer.jsp"%>
	</div> <!-- container -->
		


</body>
</html>
