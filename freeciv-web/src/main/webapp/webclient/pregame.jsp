
<div id="pregame_page">
  <div id="pregame_logo"></div>
  <div id="pregame_options">
    <div id="pregame_buttons">
      <button type="button" class="ui-state-default ui-corner-all" onClick="pregame_start_game();">Start Game</button>
      <button type="button" class="ui-state-default ui-corner-all" onClick="leave_pregame();" >Leave Game</button>
      <button id="observe_button" type="button" class="ui-state-default ui-corner-all" onClick="observe();" >Observe Game</button>
      <button type="button" class="ui-state-default ui-corner-all" onClick="pick_nation();" >Pick Nation</button>
    </div>

    <div id="share_icons">
      <a href="http://www.facebook.com/sharer.php?u=http://www.freeciv.net/games/<%= session.getAttribute("username") %>&t=Join me in a Freeciv.net multiplayer game at http://www.freeciv.net/games/<%= session.getAttribute("username") %>" target="_new">
        <img src="/images/facebook-share.png" border="0" title="Invite friends on Facebook!">
      </a>
      <a href="http://twitter.com/home?status=Join me in a Freeciv.net multiplayer game at http://www.freeciv.net/games/<%= session.getAttribute("username") %>" target="_new">
        <img src="/images/twitter-share.png" border="0" title="Invite friends on Twitter!">
      </a>

    </div>

  </div>

  
   
  <div id="pregame_player_list"></div> 
  <div id="pregame_message_area"></div>
  <div id="pregame_chat_box">
    <input id="pregame_text_input" type="text" name="text_input" onkeydown="javascript:return check_text_input(event,this);"
    			value="Type your messages here." 
    			onfocus="keyboard_input=false; if (this.value=='Type your messages here.') this.value='';" 
    			onblur="keyboard_input=true; if (this.value=='') this.value='Type your messages here.'"
    			/>
  </div>
</div>

 <div id="pick_nation_dialog" ></div>
