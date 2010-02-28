  <div>
    <h1 id="city_heading"></h1>
    <h3 id="city_size"></h3>
    
    <div id="city_left_panel" style="float: left; width: 40%;">
      <div id="city_canvas_div" style="border: 1px solid #888888;">
            <canvas id="city_canvas" width="300" height="150" moz-opaque="true"></canvas>
      
      		<div id="city_ie_canvas"></div>
      </div>
      
      <span>Improvements:</span>
      <div id="city_improvements" style="height: 100px; width: 90%; overflow: auto; background-color: #111111; border: 1px solid #444444;">
        <div id="city_improvements_list">
        </div>
      </div>
      
      <span>Units in City:</span>
      <div id="city_present_units" style="height: 100px; width: 90%; overflow: auto; background-color: #111111; border: 1px solid #444444;">
        <div id="city_present_units_list">
        </div>
      </div>
    
    </div>
    
    <div id="city_right_panel">
    
      <div id="city_production"></div>
      <div id="city_production_turns"></div>
      <div>
        <button type="button" onClick="send_city_buy();" >Buy</button>
      </div>
      <span>Change city production:</span>
      <div id="city_change_production" style="height: 330px; width: 120px; overflow: auto; border: background-color: #111111; border: 1px solid #444444;">
        <div id="city_production_choices" style="cursor:pointer;cursor:hand; width: 90px;">
        </div>
      </div>
      
      <div style="margin-top: 50px;"> 
        <button type="button" onClick="close_city_dialog();" >Close</button>
      </div>
    </div>
    
    
    
    
    
    
    
    
    
    
    
    
    
    
  
  </div>
  
  