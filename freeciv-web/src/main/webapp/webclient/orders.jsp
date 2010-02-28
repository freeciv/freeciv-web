<div id="game_unit_orders_default">       
  <div id="order_goto" class="order_button" title="Goto (Select unit, click goto, click on destination with mouse). Shortcut: G">
    <a href="#" onclick="activate_goto();" 
     onmouseover="document.images['goto_button'].src='/images/orders/goto_hover.png';"
     onmouseout="document.images['goto_button'].src='/images/orders/goto_default.png';" 
     onmousedown="document.images['goto_button'].src='/images/orders/goto_clicked.png';" 
     onmouseup="document.images['goto_button'].src='/images/orders/goto_hover.png';">
       <img src="/images/orders/goto_default.png" name="goto_button" alt="" border="0" width="30" height="50">
     </a>
  </div>

  <div id="order_explore" class="order_button" title="Auto explore. Shortcut: X">
    <a href="#" onclick="key_unit_auto_explore();" 
     onmouseover="document.images['auto_explore_button'].src='/images/orders/auto_explore_hover.png';"
     onmouseout="document.images['auto_explore_button'].src='/images/orders/auto_explore_default.png';" 
     onmousedown="document.images['auto_explore_button'].src='/images/orders/auto_explore_clicked.png';" 
     onmouseup="document.images['auto_explore_button'].src='/images/orders/auto_explore_hover.png';">
       <img src="/images/orders/auto_explore_default.png" name="auto_explore_button" alt="" border="0" width="30" height="50">
     </a>
  </div>  
    
  <div id="order_fortify" class="order_button" title="Fortify unit. Shortcut: F">
    <a href="#" onclick="key_unit_fortify();" 
     onmouseover="document.images['fortify_button'].src='/images/orders/fortify_hover.png';"
     onmouseout="document.images['fortify_button'].src='/images/orders/fortify_default.png';" 
     onmousedown="document.images['fortify_button'].src='/images/orders/fortify_clicked.png';" 
     onmouseup="document.images['fortify_button'].src='/images/orders/fortify_hover.png';">
       <img src="/images/orders/fortify_default.png" name="fortify_button" alt="" border="0" width="30" height="50">
     </a>
  </div>
    
  <div id="order_disband" class="order_button" title="Disband unit. Shortcut: Shift-D">
    <a href="#" onclick="key_unit_disband();" 
     onmouseover="document.images['disband_button'].src='/images/orders/disband_hover.png';"
     onmouseout="document.images['disband_button'].src='/images/orders/disband_default.png';" 
     onmousedown="document.images['disband_button'].src='/images/orders/disband_clicked.png';" 
     onmouseup="document.images['disband_button'].src='/images/orders/disband_hover.png';">
       <img src="/images/orders/disband_default.png" name="disband_button" alt="" border="0" width="30" height="50">
     </a>
  </div>  
  

  <div id="order_build_city" class="order_button" title="Build City. Shotcut: B">
    <a href="#" onclick="request_unit_build_city();" 
     onmouseover="document.images['build_city_button'].src='/images/orders/build_city_hover.png';"
     onmouseout="document.images['build_city_button'].src='/images/orders/build_city_default.png';" 
     onmousedown="document.images['build_city_button'].src='/images/orders/build_city_clicked.png';" 
     onmouseup="document.images['build_city_button'].src='/images/orders/build_city_hover.png';">
       <img src="/images/orders/build_city_default.png" name="build_city_button" alt="" border="0" width="30" height="50">
     </a>
  </div>  

  <div id="order_auto_settlers" class="order_button" title="Autosettlers. Shotcut: A">
    <a href="#" onclick="key_unit_auto_settle();" 
     onmouseover="document.images['auto_settlers_button'].src='/images/orders/auto_settlers_hover.png';"
     onmouseout="document.images['auto_settlers_button'].src='/images/orders/auto_settlers_default.png';" 
     onmousedown="document.images['auto_settlers_button'].src='/images/orders/auto_settlers_clicked.png';" 
     onmouseup="document.images['auto_settlers_button'].src='/images/orders/auto_settlers_hover.png';">
       <img src="/images/orders/auto_settlers_default.png" name="auto_settlers_button" alt="" border="0" width="30" height="50">
     </a>
  </div>
    
  
</div>

<div id="game_unit_orders_settlers" style="display: none;">
</div>