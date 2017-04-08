<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core"%>
<%@ taglib prefix="fn" uri="http://java.sun.com/jsp/jstl/functions"%>
<%@ include file="/WEB-INF/jsp/fragments/i18n.jsp"%>
<!DOCTYPE html>
<html lang="en">
<head>
<%@include file="/WEB-INF/jsp/fragments/head.jsp"%>

<!-- Bootstrap core CSS -->
<link rel="stylesheet" href="/css/morris.css" />
<script src="/javascript/libs/raphael-min.js"></script>
<script src="/javascript/libs/morris.min.js"></script>
<style>
	.chart {
		background-color: #fcf1e0;
	}
</style>

</head>
<body>
	<%@include file="/WEB-INF/jsp/fragments/header.jsp" %>
	<div class="container">
		<h2>Type of games played</h2>
		<div class="row chart">
			<div id="chart" style="height: 90%;"></div>
		</div>
		<div class="row">
			<div class="col-md-12">
				<div class="panel-freeciv">
					<p>Shows the number of started new games Freeciv each day in the following categories:</p>
					<ul>
						<li>Freeciv-web singleplayer 2D</li>
						<li>Freeciv-web singleplayer 3D WebGL</li>
						<li>Freeciv-web multiplayer</li>
						<li>Freeciv-web PBEM play-by-email</li>
						<li>Freeciv desktop multiplayer: games reported by meta.freeciv.org</li>
						<li>Freeciv-web hotseat games</li>
					</ul>
				</div>
			</div>
		</div>


		<script>
			var data = ${data};
		    try {
		 	 Morris.Line({
			  element: 'chart',
			  data: data,
			  xkey: 'date',
			  pointSize: 0,
			  lineWidth: 1,
			  ykeys: ['webSinglePlayer', 'webMultiPlayer', 'webPlayByEmail', 'desktopMultiplayer','webHotseat', 'webSinglePlayer3D'],
			  labels: ['Freeciv-web 2D singleplayer', 'Freeciv-web multiplayer', 'Freeciv-web PBEM', 'Freeciv desktop multiplayer', 'Freeciv-web hotseat', 'Freeciv-web 3D WebGL singleplayer']
			});
            } catch(err) {
              console.log("Problem showing score log graph: " + err);
            }
		</script>

		<!-- Site footer -->
		<%@include file="/WEB-INF/jsp/fragments/footer.jsp"%>
	</div> <!-- container -->
</body>
</html>