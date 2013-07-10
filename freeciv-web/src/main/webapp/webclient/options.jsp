  
<div>

<div style="text-align: center;">
<center>

<h2>Game Options</h2>

<div class="main_menu_buttons">
  <b>Game title:</b> <input type='text' name='metamessage_setting' id='metamessage_setting' size='28' maxlength='42'>
</div>

<div class="main_menu_buttons">
  <b>Timeout (seconds per turn):</b> <input type='number' name='timeout_setting' id='timeout_setting' size='6' length='3' min='30' max='3600' step='1'>
</div>

<div class="main_menu_buttons">
  <button id="save_button" type="button" class="button" onClick="save_game();" >Save Game</button> 	
</div>	 	 	

<div class="main_menu_buttons">
  <button id="fullscreen_button" type="button" class="button" onClick="show_fullscreen_window();" >Fullscreen</button>
</div>


<div class="main_menu_buttons">
  <button id="surrender_button" type="button" class="button" onClick="surrender_game();" >Surrender Game</button>
</div>


<div class="main_menu_buttons">
  <button id="end_button" type="button" class="button" onClick="window.location='/';" >End Game</button>
</div>

<div class="main_menu_buttons">
  <button id="show_overview_button" type="button" class="button" onClick="init_overview();" >Show overview map</button>
</div>

<div class="main_menu_buttons">
  <button id="show_unit_button" type="button" class="button" onClick="init_game_unit_panel();" >Show unit info box</button>
</div>

<div class="main_menu_buttons">
  <button id="show_message_button" type="button" class="button" onClick="init_chatbox();" >Show message box</button>
</div>

<br>

<div id="map_image"></div>

</div>

</div>

