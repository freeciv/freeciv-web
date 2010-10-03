<div>

<div style="text-align: center;">
<center>

<h2>Game Options</h2>


<div id="save_game_dialog"></div>

<div class="main_menu_buttons">
  <button id="save_button" type="button" onClick="show_savegame_dialog();" >Save Game</button>
</div>

<div class="main_menu_buttons">
  <button id="surrender_button" type="button" onClick="surrender_game();" >Surrender Game</button>
</div>


<div class="main_menu_buttons">
  <button id="end_button" type="button" onClick="window.location='/';" >End Game</button>
</div>


<br><br>

<div style="width: 500px; height: 400px; overflow: auto;">
 
		<div class="jp-playlist-player"> 
			<div class="jp-interface"> 
				<ul class="jp-controls"> 
					<li><a href="#" id="jplayer_play" class="jp-play" tabindex="1">play</a></li> 
					<li><a href="#" id="jplayer_pause" class="jp-pause" tabindex="1">pause</a></li> 
					<li><a href="#" id="jplayer_stop" class="jp-stop" tabindex="1">stop</a></li> 
					<li><a href="#" id="jplayer_volume_min" class="jp-volume-min" tabindex="1">min volume</a></li> 
					<li><a href="#" id="jplayer_volume_max" class="jp-volume-max" tabindex="1">max volume</a></li> 
					<li><a href="#" id="jplayer_previous" class="jp-previous" tabindex="1">previous</a></li> 
					<li><a href="#" id="jplayer_next" class="jp-next" tabindex="1">next</a></li> 
				</ul> 
				<div class="jp-progress"> 
					<div id="jplayer_load_bar" class="jp-load-bar"> 
						<div id="jplayer_play_bar" class="jp-play-bar"></div> 
					</div> 
				</div> 
				<div id="jplayer_volume_bar" class="jp-volume-bar"> 
					<div id="jplayer_volume_bar_value" class="jp-volume-bar-value"></div> 
				</div> 
				<div id="jplayer_play_time" class="jp-play-time"></div> 
				<div id="jplayer_total_time" class="jp-total-time"></div> 
			</div> 
			<div id="jplayer_playlist" class="jp-playlist"> 
				<ul> 
					<!-- The function displayPlayList() uses this unordered list --> 
					<li></li> 
				</ul> 
			</div> 
		</div> 
	</div>

</center>
</div>

</div>

