$(document).ready(function () { 

	(function ($) {
	
		$(function () {
			loadBlog();
			loadBestOfPlayByEmail();
			displayStore();

			if (!Detector.webgl) {
	          $("#webgl_button").addClass("disabled");
	          $("#webgl_button").html("WebGL not enabled!");
			}
		});
	
		function loadBlog() {
			// TODO: rename /fpfeed.json to /feed
			$.getJSON('/fpfeed.json', function(data) {
				var MAX_ELEMENTS = 5;
				var root = document.getElementById('latest-from-blog-articles');
				data.forEach(function (article, i) {
					if (i >= MAX_ELEMENTS) {
						return;
					}
					var item = document.createElement('li');
					var title = document.createElement('a');
					var date = document.createElement('a');
					 
					title.href = article.permalink;
					title.innerHTML = article.title;
					date.href = article.permalink;
					date.innerHTML = article.date;
					 
					item.appendChild(title);
					item.appendChild(date);
					root.appendChild(item);
				});
			}).fail(function () {
				document.getElementById('latest-from-blog').style.display = 'none';
			})
		}
	
		function loadBestOfPlayByEmail() {
		
			var clearContent = function () {
				document.getElementById('best-of-play-by-email').style.display = 'none';
			};
		
			$.getJSON('/game/play-by-email/top', function(data) {
				if (data.length === 0) {
					clearContent();
				}
				var root = document.getElementById('play-by-email-list');
				data.forEach(function (item, i) {
					var row = document.createElement('tr');
					var rank = document.createElement('td');
					var player = document.createElement('td');
					var wins = document.createElement('td');
				
					rank.innerHTML = "#" + (i + 1);
					player.innerHTML = item.player;
					wins.innerHTML = item.wins;
				
					row.appendChild(rank);
					row.appendChild(player);
					row.appendChild(wins);
					root.appendChild(row);
				});
			}).fail(clearContent);
		}

		function displayStore() {
			var ua = navigator.userAgent.toLowerCase();
			if (ua.indexOf('android') >= 0) {
				$("#google-play-store").show();
			} else if (ua.indexOf('chrome') >= 0) {
				$("#chrome-web-store").show();
			}
		}
	
	
	})($)
});