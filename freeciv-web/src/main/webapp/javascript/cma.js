/**********************************************************************
    Freeciv-web - the web version of Freeciv. https://www.freeciv.org/
    Copyright (C) 2022 FCIV.NET

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

// Governor Clipboard for copy/paste:
var _cma_val_sliders = [1,0,0,0,0,0];
var _cma_min_sliders = [0,0,0,0,0,0];
var _cma_happy_slider = 0;
var _cma_celebrate = false;
var _cma_allow_disorder = false;
var _cma_max_growth = false;
var _cma_allow_specialists = true;
var _cma_max_growth = false;


/**************************************************************************
  Init Governor tab - returns true if the tab was able to generate
**************************************************************************/
function show_city_governor_tab()
{
  // Reject cases which can't show the Governor:
  if (client_is_observer() || client.conn.playing == null) return false;
  if (!active_city) return false;
  if (city_owner_player_id(active_city) != client.conn.playing.playerno) {
    $("#city_governor_tab").html("City Governor available only for domestic cities.");
    return false;
  }
  if (typeof active_city['cm_parameter'] !== 'undefined') {
    $("#cma_food").prop('checked', active_city['cm_parameter']['factor'][0] >= 5);
    $("#cma_shield").prop('checked', active_city['cm_parameter']['factor'][1] >= 5);
    $("#cma_trade").prop('checked', active_city['cm_parameter']['factor'][2] >= 5);
    $("#cma_gold").prop('checked', active_city['cm_parameter']['factor'][3] >= 5);
    $("#cma_luxury").prop('checked', active_city['cm_parameter']['factor'][4] >= 5);
    $("#cma_science").prop('checked', active_city['cm_parameter']['factor'][5] >= 5);
  } else {
    $("#cma_food").prop('checked', false);
    $("#cma_shield").prop('checked', false);
    $("#cma_trade").prop('checked', false);
    $("#cma_gold").prop('checked', false);
    $("#cma_luxury").prop('checked', false);
    $("#cma_science").prop('checked', false);
  }
}

/**************************************************************************
  Sends new CMA parameters to the server, populated from the UI states.
**************************************************************************/
function request_new_cma(city_id)
{
  var cm_parameter = {};

  if ($("#cma_food").prop('checked')) {
    _cma_val_sliders[0] = 6;
  } else {
    _cma_val_sliders[0] = 0;
  }

  if ($("#cma_shield").prop('checked')) {
    _cma_val_sliders[1] = 6;
  } else {
    _cma_val_sliders[1] = 0;
  }

  if ($("#cma_trade").prop('checked')) {
    _cma_val_sliders[2] = 6;
  } else {
    _cma_val_sliders[2] = 0;
  }

  if ($("#cma_gold").prop('checked')) {
    _cma_val_sliders[3] = 6;
  } else {
    _cma_val_sliders[3] = 0;
  }

  if ($("#cma_luxury").prop('checked')) {
    _cma_val_sliders[4] = 6;
  } else {
    _cma_val_sliders[4] = 0;
  }

  if ($("#cma_science").prop('checked')) {
    _cma_val_sliders[5] = 6;
  } else {
    _cma_val_sliders[5] = 0;
  }

  cm_parameter['minimal_surplus'] = [..._cma_min_sliders];
  cm_parameter['require_happy'] = _cma_celebrate;
  cm_parameter['allow_disorder'] = _cma_allow_disorder;
  cm_parameter['max_growth'] = _cma_max_growth;
  cm_parameter['allow_specialists'] = _cma_allow_specialists;
  cm_parameter['factor'] = [..._cma_val_sliders];
  cm_parameter['happy_factor'] = _cma_happy_slider;

  var cma_disabled = (!$("#cma_food").prop('checked') && !$("#cma_shield").prop('checked') && !$("#cma_trade").prop('checked')
                   && !$("#cma_gold").prop('checked') && !$("#cma_luxury").prop('checked') && !$("#cma_science").prop('checked') );

  if (!cma_disabled) {
    var packet = {"pid" : packet_web_cma_set,
                  "id" : city_id,
                  "cm_parameter" : cm_parameter };
    send_request(JSON.stringify(packet));
  } else {
    var packet = {"pid" : packet_web_cma_clear,
                  "id" : city_id};
    send_request(JSON.stringify(packet));
  }
}

/**************************************************************************
  Called when user clicks button to Enable/Disable governor.
**************************************************************************/
function button_pushed_toggle_cma()
{
  request_new_cma(active_city['id']);
}
