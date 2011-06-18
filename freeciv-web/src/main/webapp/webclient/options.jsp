<%	
  String guest_mode = (String)request.getSession().getAttribute("guest_user");	 	
%>
  
<div>

<div style="text-align: center;">
<center>

<h2>Game Options</h2>


<% if ((guest_mode != null && guest_mode.equals("false"))) { %>
	 	
  <div class="main_menu_buttons">
	 	
    <button id="save_button" type="button" class="button" onClick="save_game();" >Save Game</button>
	 	
  </div>	 	 	
<% } %>

<div class="main_menu_buttons">
  <button id="fullscreen_button" type="button" class="button" onClick="show_dialog_message('Fullscreen', 'Press F11 for fullscreen mode.');" >Fullscreen</button>
</div>


<div class="main_menu_buttons">
  <button id="surrender_button" type="button" class="button" onClick="surrender_game();" >Surrender Game</button>
</div>


<div class="main_menu_buttons">
  <button id="end_button" type="button" class="button" onClick="window.location='/';" >End Game</button>
</div>

<div id="share_button_box" class="main_menu_buttons">
  <button id="share_button" type="button" class="button" onClick="prepare_share_game_map();" >View map as image</button>
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



<% if ((guest_mode != null && guest_mode.equals("true"))) { %>
	 	
<br>  <b>Please log in using OpenID for full savegame support.</b><br><br><br>
<% } %>

</div>

</div>

