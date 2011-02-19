<div>


<h2>Government</h2>

 To incite a revolution, select a new government:
<p>
 <div id="governments" style="height: 180px; width: 250px; overflow: auto;">
    <div id="governments_list" style="cursor:pointer;cursor:hand">
    </div>
  </div>
  <br>
  <button id="ok" class="button" type="button" onclick="start_revolution();">Start revolution!</button>

  <br>
<p>
<h2>Select tax, luxury and science rates</h2>

<P>
<form name="rates">
<table border="0" style="color: #ffffff;">
<tr>
  <td><span>Tax:</td>
  <td>
    <div class="slider" id="slider-tax" tabIndex="1"></div>
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
    <div class="slider" id="slider-lux" tabIndex="1"></div>
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
    <div class="slider" id="slider-sci" tabIndex="1"></div>
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
<div id="max_tax_rate" style="margin:10px;">

</div>

<button id="ok" type="button" class="button" onclick="submit_player_rates();">Change tax rates</button>



</div>

