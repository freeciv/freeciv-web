$(document).ready(function () { 
	
	(function ($) {
		
		$(function () {
			loadStatistics();
		});
		
		function loadStatistics() {
			$.getJSON('/game/statistics', function(data) {
				document.getElementById('ongoing-games').innerHTML = data.ongoing;
				var singleplayer = document.getElementById('statistics-singleplayer');
				if (singleplayer) {
					singleplayer.innerHTML = data.finished.singlePlayer; //
				}
				var multiplayer = document.getElementById('statistics-multiplayer');
				if (multiplayer) {
					multiplayer.innerHTML = data.finished.multiPlayer; //
				}
			}).fail(function () {
				var statistics = document.getElementById('statistics');
				if (statistics) {
					statistics.style.display = 'none';
				}
				
			});
		}
	})($);
});