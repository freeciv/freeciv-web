<div>


<h2>Government</h2>

 To incite a revolution, select a new government:
<p>
 <div id="governments" style="height: 150px; width: 350px; overflow: auto;">
    <div id="governments_list" style="cursor:pointer;cursor:hand">
    </div>
  </div>

<p>
<h2>Select tax, luxury and science rates</h2>

<P>
<form name="rates">
<table border="0" style="color: #ffffff;">
<tr>
  <td><span>Tax:</td>
  <td>
  <div class="slider" id="slider-tax" tabIndex="1">
   <input class="slider-input" id="slider-tax-input"
      name="slider-tax-input"/>
    </div>
  </td>
  <td>
    <div id="tax_result" style="float:left;"></div>
  </td>
  <td>
    <INPUT TYPE="CHECKBOX" NAME="lock">Lock
  </td>
</tr>
<tr>
  <td>Luxury:</td>
  <td>
  <div class="slider" id="slider-lux" tabIndex="1">
   <input class="slider-input" id="slider-lux-input"
      name="slider-lux-input"/>
    </div>
  </td>
  <td>
    <div id="lux_result" style="float:left;"></div>
  </td>
  <td>
    <INPUT TYPE="CHECKBOX" NAME="lock">Lock
  </td>

</tr>
<tr>
  <td>Science:</td>
  <td>
  <div class="slider" id="slider-sci" tabIndex="1">
   <input class="slider-input" id="slider-sci-input"
      name="slider-sci-input"/>
    </div>
  </td>
  <td>
    <div id="sci_result" style="float:left;"></div>
  </td>
  <td>
    <INPUT TYPE="CHECKBOX" NAME="lock">Lock
  </td>

</tr>


</table>
</form>

<%-- <button id="cancel" type="button" onclick="parent.close_rates_dialog();">Cancel</button> --%>
<button id="ok" type="button" onclick="submit_player_rates();">OK</button>



</div>

