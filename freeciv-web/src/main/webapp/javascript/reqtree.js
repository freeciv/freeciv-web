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

// This is a hardcoded tech-tree extracted from the Freeciv C client.
// add this to reqtree.c in the Freeciv C client:
// printf("\"%d\": {\"x\":%d, \"y\":%d}, // %s \n", node->tech, startx, starty, text);
// TODO! This really needs to be cleaned up.


var reqtree = {
"2": {"x":0, "y":0}, // Alphabet
"10": {"x":0, "y":69}, // Ceremonial Burial
"63": {"x":0, "y":183}, // Pottery
"46": {"x":0, "y":300}, // Masonry
"35": {"x":0, "y":417}, // Horseback Riding
"9": {"x":0, "y":534}, // Bronze Working
"86": {"x":0, "y":651}, // Warrior Code
"87": {"x":416, "y":0}, // Writing
"13": {"x":416, "y":79}, // Code of Laws
"55": {"x":416, "y":167}, // Mysticism
"48": {"x":416, "y":246}, // Mathematics
"45": {"x":416, "y":322}, // Map Making
"62": {"x":416, "y":395}, // Polytheism   (changed manually)
"81": {"x":416, "y":456}, // The Wheel
"20": {"x":416, "y":532}, // Currency
"38": {"x":416, "y":635}, // Iron Working
"42": {"x":667, "y":38}, // Literacy
"84": {"x":667, "y":126}, // Trade
"53": {"x":667, "y":202}, // Monarchy
"4": {"x":667, "y":290}, // Astronomy
"72": {"x":667, "y":369}, // Seafaring
"19": {"x":667, "y":500}, // Construction
"80": {"x":918, "y":29}, // The Republic
"59": {"x":918, "y":152}, // Philosophy
"56": {"x":918, "y":329}, // Navigation
"25": {"x":918, "y":425}, // Engineering
"29": {"x":918, "y":504}, // Feudalism
"8": {"x":918, "y":658}, // Bridge Building  (changed manually)
"7": {"x":1169, "y":6}, // Banking
"49": {"x":1169, "y":85}, // Medicine
"85": {"x":1169, "y":173}, // University
"60": {"x":1169, "y":249}, // Physics (fixed manually)
"54": {"x":1169, "y":310}, // Monotheism
"37": {"x":1169, "y":386}, // Invention
"12": {"x":1169, "y":605}, // Chivalry
"22": {"x":1420, "y":20}, // Economics
"11": {"x":1420, "y":104}, // Chemistry
"21": {"x":1420, "y":133}, // Democracy
"83": {"x":1420, "y":210}, // Theory of Gravity
"75": {"x":1420, "y":307}, // Steam Engine
"71": {"x":1420, "y":384}, // Sanitation
"44": {"x":1420, "y":458}, // Magnetism
"82": {"x":1420, "y":535}, // Theology
"34": {"x":1420, "y":612}, // Gunpowder
"5": {"x":1753, "y":100}, // Atomic Theory (fixed manually)
"28": {"x":1753, "y":238}, // Explosives
"65": {"x":1753, "y":383}, // Railroad
"50": {"x":1753, "y":471}, // Metallurgy
"41": {"x":1753, "y":620}, // Leadership
"36": {"x":1938, "y":170}, // Industrialization
"23": {"x":1938, "y":429}, // Electricity
"18": {"x":1938, "y":508}, // Conscription
"79": {"x":2271, "y":42}, // The Corporation
"16": {"x":2271, "y":133}, // Communism
"76": {"x":2271, "y":300}, // Steel
"68": {"x":2271, "y":491}, // Refrigeration
"78": {"x":2271, "y":570}, // Tactics
"32": {"x":2543, "y":2}, // Genetic Engineering
"67": {"x":2543, "y":93}, // Refining
"27": {"x":2543, "y":172}, // Espionage
"24": {"x":2543, "y":352}, // Electronics
"33": {"x":2543, "y":428}, // Guerilla Warfare
"43": {"x":2543, "y":504}, // Machine Tools
"3": {"x":2543, "y":616}, // Amphibious Warfare
"15": {"x":2794, "y":185}, // Combustion
"51": {"x":2794, "y":472}, // Miniaturization
"6": {"x":2977, "y":233}, // Automobile
"30": {"x":2977, "y":309}, // Flight
"47": {"x":3228, "y":170}, // Mass Production
"52": {"x":3228, "y":420}, // Mobile Warfare
"64": {"x":3228, "y":496}, // Radio
"66": {"x":3479, "y":101}, // Recycling
"57": {"x":3479, "y":193}, // Nuclear Fission
"39": {"x":3479, "y":270}, // Labor Union
"17": {"x":3479, "y":346}, // Computers
"1": {"x":3479, "y":516}, // Advanced Flight
"58": {"x":3812, "y":298}, // Nuclear Power
"69": {"x":3812, "y":384}, // Robotics
"70": {"x":3812, "y":460}, // Rocketry
"14": {"x":3812, "y":539}, // Combined Arms
"73": {"x":4228, "y":287}, // Space Flight
"40": {"x":4228, "y":378}, // Laser
"61": {"x":4479, "y":120}, // Plastics
"26": {"x":4479, "y":196}, // Environmentalism
"77": {"x":4479, "y":378}, // Superconductors
"31": {"x":4680, "y":381}, // Fusion Power
"74": {"x":4680, "y":481} // Stealth
};



// multiplayer ruleset
var reqtree_multiplayer = {
"2": {"x":0, "y":0}, // Alphabet
"10": {"x":0, "y":76}, // Ceremonial Burial
"64": {"x":0, "y":200}, // Pottery
"47": {"x":0, "y":324}, // Masonry
"36": {"x":0, "y":448}, // Horseback Riding
"9": {"x":0, "y":572}, // Bronze Working
"87": {"x":0, "y":696}, // Warrior Code
"88": {"x":416, "y":0}, // Writing
"13": {"x":416, "y":80}, // Code of Laws
"56": {"x":416, "y":170}, // Mysticism
"49": {"x":416, "y":250}, // Mathematics
"46": {"x":416, "y":330}, // Map Making
"63": {"x":416, "y":420}, // Polytheism
"82": {"x":416, "y":510}, // The Wheel
"20": {"x":416, "y":590}, // Currency
"39": {"x":416, "y":690}, // Iron Working
"43": {"x":667, "y":40}, // Literacy
"85": {"x":667, "y":134}, // Trade
"54": {"x":667, "y":216}, // Monarchy
"4": {"x":667, "y":297}, // Astronomy
"73": {"x":667, "y":379}, // Seafaring
"19": {"x":667, "y":557}, // Construction
"81": {"x":918, "y":30}, // The Republic
"60": {"x":918, "y":161}, // Philosophy
"57": {"x":918, "y":338}, // Navigation
"25": {"x":918, "y":451}, // Engineering
"29": {"x":918, "y":533}, // Feudalism
"8": {"x":918, "y":679}, // Bridge Building
"7": {"x":1169, "y":6}, // Banking
"50": {"x":1169, "y":88}, // Medicine
"86": {"x":1169, "y":182}, // University
"38": {"x":1169, "y":328}, // Invention
"61": {"x":1169, "y":410}, // Physics
"55": {"x":1169, "y":464}, // Monotheism
"12": {"x":1169, "y":630}, // Chivalry
"22": {"x":1502, "y":20}, // Economics
"11": {"x":1502, "y":110}, // Chemistry
"21": {"x":1502, "y":165}, // Democracy
"84": {"x":1502, "y":222}, // Theory of Gravity
"76": {"x":1502, "y":322}, // Steam Engine
"72": {"x":1502, "y":402}, // Sanitation
"35": {"x":1502, "y":482}, // Gunpowder
"45": {"x":1502, "y":562}, // Magnetism
"83": {"x":1502, "y":642}, // Theology
"5": {"x":1835, "y":180}, // Atomic Theory
"28": {"x":1835, "y":241}, // Explosives
"51": {"x":1835, "y":374}, // Metallurgy
"66": {"x":1835, "y":471}, // Railroad
"42": {"x":1835, "y":643}, // Leadership
"37": {"x":2036, "y":170}, // Industrialization
"23": {"x":2036, "y":435}, // Electricity
"18": {"x":2036, "y":517}, // Conscription
"80": {"x":2369, "y":33}, // The Corporation
"16": {"x":2369, "y":127}, // Communism
"77": {"x":2369, "y":302}, // Steel
"69": {"x":2369, "y":489}, // Refrigeration
"31": {"x":2369, "y":571}, // Fundamentalism
"79": {"x":2369, "y":653}, // Tactics
"33": {"x":2641, "y":1}, // Genetic Engineering
"68": {"x":2641, "y":95}, // Refining
"27": {"x":2641, "y":177}, // Espionage
"24": {"x":2641, "y":378}, // Electronics
"34": {"x":2641, "y":460}, // Guerilla Warfare
"44": {"x":2641, "y":542}, // Machine Tools
"3": {"x":2641, "y":686}, // Amphibious Warfare
"15": {"x":2892, "y":185}, // Combustion
"52": {"x":2892, "y":507}, // Miniaturization
"6": {"x":3099, "y":243}, // Automobile
"30": {"x":3099, "y":325}, // Flight
"48": {"x":3350, "y":177}, // Mass Production
"53": {"x":3350, "y":473}, // Mobile Warfare
"65": {"x":3350, "y":555}, // Radio
"67": {"x":3601, "y":71}, // Recycling
"58": {"x":3601, "y":153}, // Nuclear Fission
"40": {"x":3601, "y":280}, // Labor Union
"17": {"x":3601, "y":362}, // Computers
"1": {"x":3601, "y":578}, // Advanced Flight
"59": {"x":3934, "y":204}, // Nuclear Power
"70": {"x":3934, "y":417}, // Robotics
"71": {"x":3934, "y":502}, // Rocketry
"14": {"x":3934, "y":584}, // Combined Arms
"74": {"x":4350, "y":307}, // Space Flight
"41": {"x":4350, "y":401}, // Laser
"62": {"x":4601, "y":107}, // Plastics
"26": {"x":4601, "y":189}, // Environmentalism
"78": {"x":4601, "y":401}, // Superconductors
"32": {"x":4831, "y":401}, // Fusion Power
"75": {"x":4831, "y":516} // Stealth
};



// civ2civ3 ruleset

var reqtree_civ2civ3 = {
"2": {"x":0, "y":0}, // Alphabet
"10": {"x":0, "y":114}, // Ceremonial Burial
"46": {"x":0, "y":228}, // Masonry
"63": {"x":0, "y":342}, // Pottery
"35": {"x":0, "y":456}, // Horseback Riding
"9": {"x":0, "y":570}, // Bronze Working
"86": {"x":0, "y":684}, // Warrior Code
"87": {"x":333, "y":0}, // Writing
"13": {"x":333, "y":80}, // Code of Laws
"48": {"x":333, "y":170}, // Mathematics
"55": {"x":333, "y":250}, // Mysticism
"45": {"x":333, "y":330}, // Map Making
"62": {"x":333, "y":410}, // Polytheism
"81": {"x":333, "y":500}, // The Wheel
"20": {"x":333, "y":580}, // Currency
"38": {"x":333, "y":680}, // Iron Working
"42": {"x":584, "y":38}, // Literacy
"53": {"x":584, "y":120}, // Monarchy
"4": {"x":584, "y":249}, // Astronomy
"72": {"x":584, "y":392}, // Seafaring
"84": {"x":584, "y":493}, // Trade
"19": {"x":584, "y":621}, // Construction
"80": {"x":917, "y":29}, // The Republic
"59": {"x":917, "y":144}, // Philosophy
"54": {"x":917, "y":261}, // Monotheism
"44": {"x":917, "y":343}, // Magnetism
"8": {"x":917, "y":581}, // Bridge Building
"29": {"x":917, "y":635}, // Feudalism
"85": {"x":1250, "y":51}, // University
"7": {"x":1250, "y":162}, // Banking
"82": {"x":1250, "y":244}, // Theology
"60": {"x":1250, "y":380}, // Physics
"49": {"x":1250, "y":435}, // Medicine
"37": {"x":1250, "y":492}, // Invention
"12": {"x":1250, "y":662}, // Chivalry
"22": {"x":1522, "y":79}, // Economics
"83": {"x":1522, "y":173}, // Theory of Gravity
"11": {"x":1522, "y":255}, // Chemistry
"21": {"x":1522, "y":305}, // Democracy
"56": {"x":1522, "y":402}, // Navigation
"34": {"x":1522, "y":531}, // Gunpowder
"71": {"x":1522, "y":613}, // Sanitation
"50": {"x":1855, "y":235}, // Metallurgy
"75": {"x":1855, "y":317}, // Steam Engine
"28": {"x":1855, "y":399}, // Explosives
"41": {"x":1855, "y":596}, // Leadership
"23": {"x":2106, "y":228}, // Electricity
"65": {"x":2106, "y":273}, // Railroad
"18": {"x":2106, "y":493}, // Conscription
"68": {"x":2357, "y":159}, // Refrigeration
"36": {"x":2357, "y":241}, // Industrialization
"25": {"x":2357, "y":323}, // Engineering
"78": {"x":2357, "y":544}, // Tactics
"5": {"x":2608, "y":66}, // Atomic Theory
"79": {"x":2608, "y":112}, // The Corporation
"76": {"x":2608, "y":261}, // Steel
"67": {"x":2608, "y":355}, // Refining
"16": {"x":2608, "y":437}, // Communism
"3": {"x":2608, "y":519}, // Amphibious Warfare
"24": {"x":2880, "y":67}, // Electronics
"32": {"x":2880, "y":149}, // Genetic Engineering
"15": {"x":2880, "y":325}, // Combustion
"43": {"x":2880, "y":407}, // Machine Tools
"33": {"x":2880, "y":501}, // Guerilla Warfare
"27": {"x":2880, "y":583}, // Espionage
"51": {"x":3131, "y":196}, // Miniaturization
"6": {"x":3131, "y":293}, // Automobile
"30": {"x":3131, "y":477}, // Flight
"47": {"x":3338, "y":149}, // Mass Production
"64": {"x":3338, "y":272}, // Radio
"52": {"x":3338, "y":408}, // Mobile Warfare
"57": {"x":3589, "y":78}, // Nuclear Fission
"17": {"x":3589, "y":236}, // Computers
"1": {"x":3589, "y":413}, // Advanced Flight
"66": {"x":3589, "y":495}, // Recycling
"39": {"x":3589, "y":577}, // Labor Union
"58": {"x":3922, "y":102}, // Nuclear Power
"69": {"x":3922, "y":298}, // Robotics
"70": {"x":3922, "y":380}, // Rocketry
"14": {"x":3922, "y":462}, // Combined Arms
"40": {"x":4255, "y":141}, // Laser
"61": {"x":4255, "y":223}, // Plastics
"73": {"x":4255, "y":308}, // Space Flight
"77": {"x":4506, "y":183}, // Superconductors
"74": {"x":4506, "y":265}, // Stealth
"26": {"x":4506, "y":408}, // Environmentalism
"31": {"x":4757, "y":404} // Fusion Power

};