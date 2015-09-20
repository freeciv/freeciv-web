/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2015  The Freeciv-web project

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************/

var s_tax = null;
var s_lux = null;
var s_sci = null;

var tax;
var sci;
var lux;

var maxrate = 80;
var freeze = false;
var government_list;
var current_government;


/**************************************************************************
  ...
**************************************************************************/
function show_tax_rates_dialog()
{

  var id = "#rates_dialog";
  $(id).remove();
  $("<div id='rates_dialog'></div>").appendTo("div#game_page");

  var dhtml = "<div> <p><h2>Select tax, luxury and science rates</h2>"
    + "<P><form name='rates'><table border='0' style='color: #ffffff;'>"
    + "<tr> <td><span>Tax:</td> <td> <div class='slider' id='slider-tax' tabIndex='1'></div>"
    + "</td><td>"
    + "<div id='tax_result' style='float:left;'></div></td>"
    + "<td> <INPUT TYPE='CHECKBOX' NAME='lock'>Lock</td></tr>"
    + "<tr><td>Luxury:</td><td><div class='slider' id='slider-lux' tabIndex='1'></div>"
    + "</td><td> <div id='lux_result' style='float:left;'></div>"
    + "</td><td><INPUT TYPE='CHECKBOX' NAME='lock'>Lock</td></tr>"
    + "<tr><td>Science:</td><td><div class='slider' id='slider-sci' tabIndex='1'></div>"
    + "</td><td><div id='sci_result' style='float:left;'></div>"
    + "</td><td><INPUT TYPE='CHECKBOX' NAME='lock'>Lock</td></tr>"
    + "</table></form>"
    + "<div id='max_tax_rate' style='margin:10px;'>"
    + "</div><span id='rates_info' style='margin:10px;'></div>";

  $(id).html(dhtml);

  $(id).attr("title", "Change your tax rates!");
  $(id).dialog({
			bgiframe: true,
			modal: true,
			width: "450",
			  buttons: {
				"Close" : function() {
					$("#rates_dialog").dialog('close');
				}
			  }

  });

  update_rates_dialog();

}

/**************************************************************************
  ...
**************************************************************************/
function update_rates_dialog()
{

  if (client_is_observer()) return;

  maxrate = government_max_rate(client.conn.playing['government']);

  $("#slider-tax").html("<input class='slider-input' id='slider-tax-input' name='slider-tax-input'/>");
  $("#slider-lux").html("<input class='slider-input' id='slider-lux-input' name='slider-lux-input'/>");
  $("#slider-sci").html("<input class='slider-input' id='slider-sci-input' name='slider-sci-input'/>");

  create_rates_dialog(client.conn.playing['tax'],
                      client.conn.playing['luxury'],
                      client.conn.playing['science'], maxrate);

  var govt = governments[client.conn.playing['government']];

  $("#max_tax_rate").html("<i>" + govt['name'] + " max rate: " + maxrate + "</i>");
  update_net_income();
}

/**************************************************************************
  ...
**************************************************************************/
function update_net_income()
{
  var net_income = client.conn.playing['net_income'];
  if (client.conn.playing['net_income'] > 0) {
    net_income = "+" + client.conn.playing['net_income'];
  }
  $("#rates_info").html("Net income: " + net_income);
}

/**************************************************************************
  ...
**************************************************************************/
function create_rates_dialog(tax, lux, sci, max)
{
  s_tax = new Slider(document.getElementById("slider-tax"),
                   document.getElementById("slider-tax-input"));
  s_tax.setValue(tax);
  s_tax.setMaximum(max);
  s_tax.setMinimum(0);
  s_tax.setBlockIncrement(10);
  s_tax.setUnitIncrement(10);
  s_tax.onchange = update_tax_rates;

  s_lux = new Slider(document.getElementById("slider-lux"),
                   document.getElementById("slider-lux-input"));
  s_lux.setValue(lux);
  s_lux.setMaximum(max);
  s_lux.setMinimum(0);
  s_lux.setBlockIncrement(10);
  s_lux.setUnitIncrement(10);
  s_lux.onchange = update_lux_rates;


  s_sci = new Slider(document.getElementById("slider-sci"),
                   document.getElementById("slider-sci-input"));
  s_sci.setValue(sci);
  s_sci.setMaximum(max);
  s_sci.setMinimum(0);
  s_sci.setBlockIncrement(10);
  s_sci.setUnitIncrement(10);
  s_sci.onchange = update_sci_rates;

  maxrate = max ;

  update_rates_labels();

}

/**************************************************************************
  ...
**************************************************************************/
function
update_rates_labels ()
{

  tax = s_tax.getValue();
  lux = s_lux.getValue();
  sci = s_sci.getValue();

  $("#tax_result").html(tax + "%");
  $("#lux_result").html(lux + "%");
  $("#sci_result").html(sci + "%");


}

/**************************************************************************
  ...
**************************************************************************/
function update_tax_rates ()
{
  if (freeze) return;
  freeze = true;

  if (s_tax.getValue() % 10 != 0) s_tax.setValue(s_tax.getValue() - (s_tax.getValue() % 10));
  if (s_lux.getValue() % 10 != 0) s_lux.setValue(s_lux.getValue() - (s_lux.getValue() % 10));
  if (s_sci.getValue() % 10 != 0) s_sci.setValue(s_sci.getValue() - (s_sci.getValue() % 10));

  lock_tax = document.rates.lock[0].checked;
  lock_lux = document.rates.lock[1].checked;
  lock_sci = document.rates.lock[2].checked;

  tax = s_tax.getValue();
  lux = s_lux.getValue();
  sci = s_sci.getValue();

  if (tax + lux + sci  != 100 && lock_lux == false) {
    lux = Math.min(Math.max(100 - tax - sci, 0), maxrate);
  }
  if (tax + lux + sci  != 100 && lock_sci == false) {
    sci = Math.min(Math.max(100 - lux - tax, 0), maxrate);
  }

  if (tax + lux + sci  != 100) {
    s_tax.setValue(100 - lux - sci);
    freeze = false;
    return;
  }

  s_tax.setValue(tax);
  s_lux.setValue(lux);
  s_sci.setValue(sci);

  $("#tax_result").html(tax + "%");

  $("#lux_result").html(lux + "%");

  $("#sci_result").html(sci + "%");

  freeze = false;
  submit_player_rates();
}

/**************************************************************************
  ...
**************************************************************************/
function update_lux_rates ()
{
  if (freeze) return;
  freeze = true;

  if (s_tax.getValue() % 10 != 0) s_tax.setValue(s_tax.getValue() - (s_tax.getValue() % 10));
  if (s_lux.getValue() % 10 != 0) s_lux.setValue(s_lux.getValue() - (s_lux.getValue() % 10));
  if (s_sci.getValue() % 10 != 0) s_sci.setValue(s_sci.getValue() - (s_sci.getValue() % 10));

  lock_tax = document.rates.lock[0].checked;
  lock_lux = document.rates.lock[1].checked;
  lock_sci = document.rates.lock[2].checked;

  tax = s_tax.getValue();
  lux = s_lux.getValue();
  sci = s_sci.getValue();

  if (tax + lux + sci  != 100 && lock_tax == false) {
    tax = Math.min(Math.max(100 - lux - sci, 0), maxrate);
  }
  if (tax + lux + sci  != 100 && lock_sci == false) {
    sci = Math.min(Math.max(100 - lux - tax, 0), maxrate);
  }

  if (tax + lux + sci  != 100) {
    s_lux.setValue(100 - tax - sci);
    freeze = false;
    return;
  }

  s_tax.setValue(tax);
  s_lux.setValue(lux);
  s_sci.setValue(sci);

  $("#tax_result").html(tax + "%");
  $("#lux_result").html(lux + "%");
  $("#sci_result").html(sci + "%");


  freeze = false;
  submit_player_rates();

}

/**************************************************************************
  ...
**************************************************************************/
function update_sci_rates ()
{
  if (freeze) return;
  freeze = true;

  if (s_tax.getValue() % 10 != 0) s_tax.setValue(s_tax.getValue() - (s_tax.getValue() % 10));
  if (s_lux.getValue() % 10 != 0) s_lux.setValue(s_lux.getValue() - (s_lux.getValue() % 10));
  if (s_sci.getValue() % 10 != 0) s_sci.setValue(s_sci.getValue() - (s_sci.getValue() % 10));

  lock_tax = document.rates.lock[0].checked;
  lock_lux = document.rates.lock[1].checked;
  lock_sci = document.rates.lock[2].checked;

  tax = s_tax.getValue();
  lux = s_lux.getValue();
  sci = s_sci.getValue();

  if (tax + lux + sci  != 100 && lock_lux == false) {
    lux = Math.min(Math.max(100 - tax - sci, 0), maxrate);
  }
  if (tax + lux + sci  != 100 && lock_tax == false) {
    tax = Math.min(Math.max(100 - sci - lux, 0), maxrate);
  }

  if (tax + lux + sci  != 100) {
    s_sci.setValue(100 - lux - tax);
    freeze = false;
    return;
  }

  s_tax.setValue(tax);
  s_lux.setValue(lux);
  s_sci.setValue(sci);

  $("#tax_result").html(tax + "%");
  $("#lux_result").html(lux + "%");
  $("#sci_result").html(sci + "%");

  freeze = false;
  submit_player_rates();

}

/**************************************************************************
  ...
**************************************************************************/
function submit_player_rates()
{
  if (tax >= 0 && tax <= 100 && lux >= 0 && lux <= 100 && sci >= 0 && sci <= 100) {
    var packet = {"pid" : packet_player_rates,
                  "tax" : tax, "luxury" : lux, "science" : sci };
    send_request(JSON.stringify(packet));
  } else {
    swal("Invalid tax rate values");
  }
}
