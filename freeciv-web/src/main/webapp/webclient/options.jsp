  
<div>

<div style="text-align: center;">
<center>

<h2>Game Options</h2>

<div class="main_menu_buttons">
<b>Music:</b><br>
  <audio preload="none"></audio>
</div>

<div class="main_menu_buttons">
<table>
<tr>
  <td>
    <button id="switch_renderer_button" type="button" class="button setting_button" onClick="switch_renderer();"></button>
  </td>
  <td>
    <div id="renderer_help" style="font-size: 85%; max-width: 450px;"></div>
  </td>
</tr>
<tr>
  <td>
    <button id="replay_button" type="button" class="button setting_button" onClick="show_replay();">Show game replay</button>
  </td>
  <td>
    Show game replay
  </td>
</tr>
</table>

</div>


<div class="main_menu_buttons" id="timeout_setting_div">
  <b>Timeout (seconds per turn):</b> <input type='number' name='timeout_setting' id='timeout_setting' size='6' length='3' max='3600' step='1'>
  <span id="timeout_info"></span>
</div>


<table>
<tr>
<td>
  <div class="main_menu_buttons">
    <b>Play sounds:</b> <input type='checkbox' name='play_sounds_setting' id='play_sounds_setting' checked>
  </div>
</td>
<td>
  <div class="main_menu_buttons">
    <b>Speech messages:</b> <input type='checkbox' name='speech_enabled_setting' id='speech_enabled_setting'>
  </div>
</td>
</tr>

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
<tr>
<td>
<div class="main_menu_buttons">
  <button id="update_model_button" type="button" class="button setting_button" onClick="update_webgl_model();" title="Update a webgl model" >Update 3D model</button>
</div>
</td>
<td>
</td>
</tr>
</table>


<div class="main_menu_buttons" id="title_setting_div">
  <b>Game title:</b> <input type='text' name='metamessage_setting' id='metamessage_setting' size='28' maxlength='42'>
</div>


</center>
</div>

</div>

