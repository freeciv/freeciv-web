  
<div>

<div style="text-align: center;">
<center>

<h2>Game Options</h2>

<div class="main_menu_buttons">
  <b>Game title:</b> <input type='text' name='metamessage_setting' id='metamessage_setting' size='28' maxlength='42'>
</div>

<div class="main_menu_buttons">
<b>Music:</b><br>
  <audio preload="none"></audio>
</div>

<div class="main_menu_buttons">
  <b>Play sounds:</b> <input type='checkbox' name='play_sounds_setting' id='play_sounds_setting' checked>
</div>

<div class="main_menu_buttons">
  <b>Speech messages:</b> <input type='checkbox' name='speech_enabled_setting' id='speech_enabled_setting'>
</div>

<div class="main_menu_buttons">
  <b>Timeout (seconds per turn):</b> <input type='number' name='timeout_setting' id='timeout_setting' size='6' length='3' min='30' max='3600' step='1'>
</div>


<table>
<tr>
<td>
<div class="main_menu_buttons">
  <button id="save_button" type="button" class="button setting_button" onClick="save_game();" title="Saves your current game so you can continue later. Press Ctrl+S to quick save the game.">Save Game (Ctrl+S)</button>
</div>
</td>
<td>
<div class="main_menu_buttons">
  <button id="fullscreen_button" type="button" class="button setting_button" onClick="show_fullscreen_window();" title="Enables fullscreen window mode" >Fullscreen</button>
</div>
</td>
</tr>
<tr>
<td>
<div class="main_menu_buttons">
  <button id="surrender_button" type="button" class="button setting_button" onClick="surrender_game();" title="Surrenders in multiplayer games and thus ends the game for you.">Surrender Game</button>
</div>
</td>
<td>
<div class="main_menu_buttons">
  <button id="end_button" type="button" class="button setting_button" onClick="window.location='/';" title="Ends the game, and returns to the main page of Freeciv-web." >End Game</button>
</div>
</td>
</tr>
</table>


</div>

</div>

