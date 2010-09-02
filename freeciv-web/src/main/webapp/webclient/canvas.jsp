 <div>
    <div id="game_status_panel">
    </div>
    
    <div id="canvas_div">
      <canvas id="canvas" width="1024" height="768" onmousedown="mapview_mouse_click(event)" moz-opaque="true"></canvas>
    </div>
    
    <div id="game_info_panel">
      <div id="game_overview_map" onclick="overview_clicked(event);">
        <div id="map_click_div">
          
          <SCRIPT><!--
          
          function getInternetExplorerVersion()
			// Returns the version of Internet Explorer or a -1
			// (indicating the use of another browser).
			{
			  var rv = -1; // Return value assumes failure.
			  if (navigator.appName == 'Microsoft Internet Explorer')
			  {
			    var ua = navigator.userAgent;
			    var re  = new RegExp("MSIE ([0-9]{1,}[\.0-9]{0,})");
			    if (re.exec(ua) != null)
			      rv = parseFloat( RegExp.$1 );
			  }
			  return rv;
			}
           
            if (navigator.appName == 'Microsoft Internet Explorer' && getInternetExplorerVersion() < 8) {
             // Please support W3 standards or go bankrupt.  Your choice.
             // In the mean time, use slow table method.
             document.write('<TABLE CELLPADDING=0 CELLSPACING=0 ID="overview_map"></TABLE>');
           } else {
             document.write('<img id="overview_map"/>');
           }
           //--></SCRIPT>          
        </div>
      </div>
      <div id="game_communication_panel">
        <div id="game_message_area"></div>
        <div id="game_chat_box">
          <input id="game_text_input" type="text" name="text_input" onkeydown="javascript:return check_text_input(event,this);"
            							value="Type your messages here." 
                                        onfocus="keyboard_input=false; if (this.value=='Type your messages here.') this.value='';" 
    									value="Type your messages here."
                                         />
        </div>
      </div>
      <div id="game_action_panel">&nbsp;
        <jsp:include page="orders.jsp" flush="false"/>
        <div id="game_unit_info">&nbsp;
        </div>
      </div>
    </div>
  </div>
