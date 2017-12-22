/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2017  The Freeciv-web project

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

var speech_recogntition_enabled = false;
var recognition = null;

/**************************************************************************
 is the Speech Regogntition API enabled?
**************************************************************************/
function is_speech_recognition_supported()
{
  if ('SpeechRecognition' in window || 'webkitSpeechRecognition' in window ) {
    return true;
  } else {
    return false;
  }
}

/****************************************************************************
...
****************************************************************************/
function speech_recogntition_init()
{
  recognition = new (window.SpeechRecognition || window.webkitSpeechRecognition || window.mozSpeechRecognition || window.msSpeechRecognition)();
  recognition.lang = 'en-US';
  recognition.interimResults = false;

  var grammar = '#JSGF V1.0; grammar commands; public <command> =  y | o | b | x | a | f | m | u  | d | l | r | n | e | w | t | yes | no | build | city | explore | road | irrigation | auto | mine | up | down | right | left | north | south | east | west | northeast | northwest | southeast | southwest | done | turn | end ;'
  var speechRecognitionList = new (window.SpeechGrammarList || window.webkitSpeechGrammarList)();
  speechRecognitionList.addFromString(grammar, 1);
  recognition.grammars = speechRecognitionList;

  recognition.maxAlternatives = 1;
  recognition.start();

  recognition.onresult = speech_recogntition_handle_result;

  recognition.onend = function(event) {
    recognition.start();
  };
}



/****************************************************************************
...
****************************************************************************/
function speech_recogntition_handle_result(event)
{
  var found = false;
  for (var i = 0; i < event.results.length; i++) {
    var spoken_command = event.results[i][0].transcript;
    var test_words = [spoken_command.toLowerCase() , spoken_command.substring(0,1).toLowerCase()];
    for (var i = 0; i < test_words.length; i++) {
     switch (test_words[i]) {
      case "y":
      case "o":
      case "yes":
      case "ok":
        var e = jQuery.Event("keypress");
        e.which = 13;
        e.keyCode = 13;
        $(document).trigger(e);

        found = true;
        break;

      case "no":
        var e = jQuery.Event("keypress");
        e.which = 27;
        e.keyCode = 27;
        $(document).trigger(e);

        found = true;
        break;

      case "b":
      case "build":
      case "city":
        request_unit_build_city();
        found = true;
        break;

      case "x":
        key_unit_auto_explore();
        found = true;
        break;

      case "road":
        key_unit_road();
        found = true;
        break;

      case "i":
      case "irrigation":
        key_unit_irrigation();
        found = true;
        break;

      case "a":
      case "auto":
        key_unit_auto_settle();
        found = true;
        break;

      case "f":
      case "fortify":
        key_unit_fortify();
        found = true;
        break;

      case "m":
      case "mine":
        key_unit_mine();
        found = true;
        break;

      case "wait":
        key_unit_wait();
        found = true;
        break;

      case "u":
      case "up":
        key_unit_move(DIR8_NORTHWEST);
        found = true;
        break;

      case "d":
      case "down":
        key_unit_move(DIR8_SOUTHEAST);
        found = true;
        break;

      case "l":
      case "left":
        key_unit_move(DIR8_SOUTHWEST);
        found = true;
        break;

      case "r":
      case "right":
        key_unit_move(DIR8_NORTHEAST);
        found = true;
        break;

      case "n":
      case "north":
        key_unit_move(DIR8_NORTH);
        found = true;
        break;

      case "s":
      case "south":
        key_unit_move(DIR8_SOUTH);
        found = true;
        break;

      case "e":
      case "east":
        key_unit_move(DIR8_EAST);
        found = true;
        break;

      case "w":
      case "west":
        key_unit_move(DIR8_WEST);
        found = true;
        break;

      case "north east":
      case "northeast":
        key_unit_move(DIR8_NORTHEAST);
        found = true;
        break;

      case "nort west":
      case "nortwest":
        key_unit_move(DIR8_NORTHWEST);
        found = true;
        break;

      case "south west":
      case "southwest":
        key_unit_move(DIR8_SOUTHWEST);
        found = true;
        break;

      case "south east":
      case "southeast":
        key_unit_move(DIR8_SOUTHEAST);
        found = true;
        break;

      case "t":
      case "turn":
        send_end_turn();
        found = true;
        break;


     }
    }
  }

  if (!found && speech_enabled) {
    //speak("Unknown command " + event.results[0][0].transcript)
  }


}
