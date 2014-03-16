
<div id="pregame_page">
  <div id="pregame_options">
	<div id="pregame_buttons">
		<div id="freeciv_logo" style="cursor:pointer;cursor:hand" 
			  onclick="window.open('http://play.freeciv.org/', '_new');">
		</div>
		<button id="start_game_button" type="button" class="button" onClick="pregame_start_game();">Start Game</button>
		<button id="load_game_button" type="button" class="button" onClick="load_game_dialog();" >Load Game</button>
		<button id="observe_button" type="button" class="button" onClick="observe();" >Observe Game</button>
		<button id="pick_nation_button" type="button" class="button" onClick="pick_nation();" >Pick Nation</button>
		<button id="pregame_settings_button" type="button" class="button" onClick="pregame_settings();" >Settings</button>
	</div>

  </div>

  
   
  <div id="pregame_player_list"></div> 
  <div id="pregame_message_area"></div>
  <div id="pregame_chat_box">
    <input id="pregame_text_input" type="text" name="text_input" onkeydown="javascript:return check_text_input(event,this);"
    			value=">" 
			onfocus="keyboard_input=false; if (this.value=='>') this.value='';" 
			onblur="keyboard_input=true; if (this.value=='') this.value='>'"
    			/>
  </div>
</div>

 <div id="pick_nation_dialog" ></div>
