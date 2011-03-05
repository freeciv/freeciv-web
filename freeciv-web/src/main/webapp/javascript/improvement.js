/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

var improvements = {};


/**************************************************************************
 Returns a list containing improvements which are available from a tech.
**************************************************************************/
function get_improvements_from_tech(tech_id)
{
  var result = [];
  for (improvement_id in improvements) {
    var pimprovement = improvements[improvement_id];
    if (get_improvement_requirements(improvement_id) == tech_id) {
      result.push(pimprovement);
    }
  }
  return result;

}



/**************************************************************************
   
   //printf("\"%d\":%d,\n", pimprove->item_number, node->tech);
**************************************************************************/
function get_improvement_requirements(improvement_id)
{
var req_map = {
"37":10,
"12":63,
"48":63,
"7":46,
"21":46,
"47":46,
"60":46,
"41":9,
"15":87,
"10":13,
"59":55,
"54":45,
"16":20,
"46":42,
"57":84,
"42":4,
"13":72,
"1":19,
"9":19,
"55":56,
"52":25,
"64":29,
"2":7,
"62":49,
"38":85,
"6":54,
"58":54,
"53":37,
"34":22,
"40":22,
"63":21,
"50":83,
"45":75,
"29":71,
"51":82,
"4":34,
"8":34,
"44":65,
"11":36,
"66":36,
"22":16,
"65":16,
"36":68,
"43":32,
"24":67,
"14":24,
"49":24,
"23":3,
"20":51,
"35":6,
"17":47,
"5":52,
"0":64,
"25":66,
"56":57,
"26":17,
"61":17,
"19":58,
"18":69,
"27":70,
"33":73,
"39":73,
"28":40,
"31":61,
"30":26,
"32":77};

return req_map[improvement_id + ''];
}
