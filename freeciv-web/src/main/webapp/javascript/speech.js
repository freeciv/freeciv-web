/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2016  The Freeciv-web project

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

/* Speech Synthesis API  (text to speech)*/

var speech_enabled = false;
var voice = null;

/**************************************************************************
 is Speech Synthesis API enabled?
**************************************************************************/
function is_speech_supported()
{
  if ('speechSynthesis' in window) {
    return true;
  } else {
    return false;
  }
}

/**************************************************************************
 Say the given text using the Speech Synthesis API
**************************************************************************/
function speak(text) {
  if (!is_speech_supported() || !speech_enabled) return;
  speech_synth(text, true);

}

/**************************************************************************
 Say the given text using the Speech Synthesis API, without filter.
**************************************************************************/
function speak_unfiltered(text) {
  if (!is_speech_supported() || !speech_enabled) return;
  speech_synth(text, false);

}

/**************************************************************************
 Say the given text using the Speech Synthesis API
**************************************************************************/
function speech_synth(text, filter_enabled) {
  if (!is_speech_supported() || !speech_enabled) return;

    var text_filtered = filter_enabled ? filter_speech_messages(text) : text;
    if (text_filtered == null) return;

	var msg = new SpeechSynthesisUtterance();
 	msg.text = text_filtered;

	msg.volume = parseFloat(1);
	msg.rate = parseFloat(1.1);
	msg.pitch = parseFloat(1);
	msg.lang = 'en-US';

	if (voice == null) {
      var voices = speechSynthesis.getVoices();
      voices.forEach(function(voice_check, i) {
        // try to find a default voice
        if (voice_check.name == "Google UK English Female") voice = "Google UK English Female";
      });
	}

	if (voice != null) {
	  msg.voice = speechSynthesis.getVoices().filter(function(voice_check) { return voice_check.name == voice; })[0];
	}

	window.speechSynthesis.speak(msg);

}

/**************************************************************************
  Filter away parts of the messages which would not be suited for
  text-to-speech pronunciation.
**************************************************************************/
function filter_speech_messages(input) {
  if (C_S_RUNNING != client_state()) return null;
  if (input == null || input.length == 0) return null;
  if (input.indexOf("metamessage") != -1) return null;
  if (input.indexOf("rules the") != -1) return null;

  input = input.replace(/(\r\n|\n|\r)/gm,"");
  input = input.replace(/AI/g, '');
  input = input.replace(/\*/g, '');

  // strip HTML from the message
  input = jQuery("<div>" + input + "</div>").text();

  if (input.length > 90) return null;

  return input;

}

/**************************************************************************
...
**************************************************************************/
function load_voices() {
  if (!is_speech_supported()) return;
  var voices = speechSynthesis.getVoices();
  var voiceSelect = document.getElementById('voice');
  if (voiceSelect == null) return;
  voices.forEach(function(voice, i) {
      var option = document.createElement('option');
      option.value = voice.name;
      option.innerHTML = voice.name;
      voiceSelect.appendChild(option);
  });

}