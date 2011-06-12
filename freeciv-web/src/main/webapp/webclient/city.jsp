  <div id="city_viewport">
    <h1 id="city_heading"></h1>
    <h3 id="city_size"></h3>
    
    <div id="city_left_panel">
      <div id="city_canvas_div">
            <canvas id="city_canvas" width="300" height="150" onmousedown="city_mapview_mouse_click(event)" moz-opaque="true"></canvas>
      </div>

      <div id="city_dialog_info">
	<b>City information:</b>
	<table>
	<tr><td>Food: </td><td id="city_food"></td></tr>
	<tr><td>Prod: </td><td id="city_prod"></td></tr>
	<tr><td>Trade: </td><td id="city_trade"></td></tr>
	<tr><td>Gold:: </td><td id="city_gold"></td></tr>
	<tr><td>Luxury: </td><td id="city_luxury"></td></tr>
	<tr><td>Science: </td><td id="city_science"></td></tr>

	</table>
      </div>
      <div id="city_button_pane">
        <button type="button" class="button" onClick="close_city_dialog();" >Close</button>
        <button type="button" class="button" onClick="next_city();" >Next city</button>

      </div>

      <span>Improvements:</span>
      <div id="city_improvements">
        <div id="city_improvements_list">
        </div>
      </div>
      
      <span>Units in City:</span>
      <div id="city_present_units" >
        <div id="city_present_units_list">
        </div>
      </div>
    
    </div>
    
    <div id="city_right_panel">
    
      <div id="city_production"></div>
      <div id="city_production_turns"></div>
      <div>
        <button type="button" class="button" onClick="send_city_buy();" >Buy</button>
      </div>
      <span>Change city production:</span>
      <div id="city_change_production">
        <div id="city_production_choices">
        </div>
      </div>
      
    </div>
    
    
    
    
    
    
    
    
    
    
    
    
    
    
  
  </div>
  
  
