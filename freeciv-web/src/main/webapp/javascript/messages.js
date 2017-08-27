/**********************************************************************
    Copyright (C) 2017  The Freeciv-web project

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

var chatbox_active = true;
var chatbox_text = " ";
var chatbox_text_dirty = false;
var previous_scroll = 0;
var chatbox_panel_toggle = false;

/**************************************************************************
 ...
**************************************************************************/
function init_chatbox()
{

  chatbox_active = true;

  $("#game_chatbox_panel").dialog({
			bgiframe: true,
			modal: false,
			width: "27%",
			height: (is_small_screen() ? 100 : 200),
			resizable: true,
			dialogClass: 'chatbox_dialog no-close',
			closeOnEscape: false,
			position: {my: 'left bottom', at: 'left bottom', of: window, within: $("#game_page")},
			close: function(event, ui) { chatbox_active = false;}
		});

  $("#game_chatbox_panel").dialog('open');
  $(".chatbox_dialog").css("top", "52px");


  if (is_small_screen()) {
    $(".chatbox_dialog").css("left", "2px");
    $(".chatbox_dialog").css("top", "40px");
    $("#game_chatbox_panel").parent().css("max-height", "15%");
    $("#game_chatbox_panel").parent().css("width", "95%");

    $("#game_message_area").click(function(e) {
      if (chatbox_panel_toggle) {
        $("#game_chatbox_panel").parent().css("max-height", "15%");
        $("#game_chatbox_panel").parent().css("height", "15%");
      } else {
        $("#game_chatbox_panel").parent().css("max-height", "65%");
        $("#game_chatbox_panel").parent().css("height", "65%");
      }
      chatbox_panel_toggle = !chatbox_panel_toggle;
    });
  }
}

/**************************************************************************
 This adds new text to the main message chatbox. See update_chatbox() which
 does the actual update to the screen.
**************************************************************************/
function add_chatbox_text(text)
{
    if (civclient_state <= C_S_PREPARING) {
      text = text.replace(/#FFFFFF/g, '#000000');
    } else {
      text = text.replace(/#0000FF/g, '#5555FF')
                 .replace(/#006400/g, '#00AA00')
                 .replace(/#551166/g, '#AA88FF')
                 .replace(/#A020F0/g, '#F020FF');
    }

    if (is_longturn() && text != null && (text.indexOf("waiting on") != -1 || text.indexOf("Lost connection") != -1 || text.indexOf("Not enough") != -1 || text.indexOf("has been removed") != -1 || text.indexOf("has connected") != -1)) return;
    if (text != null && text.indexOf("Year:") != -1) text = "<hr style='border-color: #555555;'>" + text;
    if (!check_text_with_banlist(text)) return;

    chatbox_text += text + "<br>";
    chatbox_text_dirty = true;

}


/**************************************************************************
 This is called at regular time intervals to update the chatbox text window
 with new messages. It is updated at interals for performance reasons
 when many messages appear.
**************************************************************************/
function update_chatbox()
{
  var scrollDiv;
  if (civclient_state <= C_S_PREPARING) {
      scrollDiv = document.getElementById('pregame_message_area');
  } else {
      scrollDiv = document.getElementById('game_message_area');
  }

  if (chatbox_text_dirty && scrollDiv != null && chatbox_text != null) {
      scrollDiv.innerHTML = chatbox_text;

      var currentHeight = 0;

      if (scrollDiv.scrollHeight > 0) {
        currentHeight = scrollDiv.scrollHeight;
      } else if (scrollDiv.offsetHeight > 0) {
        currentHeight = scrollDiv.offsetHeight;
      }

      if (previous_scroll < currentHeight) {
        scrollDiv.scrollTop = currentHeight;
      }

      previous_scroll = currentHeight;
      chatbox_text_dirty = false;
  }

}

/**************************************************************************
 Clips the chatbox text to a maximum number of lines.
**************************************************************************/
function chatbox_clip_messages()
{
  var max_chatbox_lines = 24;

  var new_chatbox_text = "";
  var chat_lines = chatbox_text.split("<br>");
  if (chat_lines.length <=  max_chatbox_lines) return;
  for (var i = chat_lines.length - max_chatbox_lines; i < chat_lines.length; i++) {
    new_chatbox_text += chat_lines[i];
    if ( i < chat_lines.length - 1) new_chatbox_text += "<br>";
  }

  chatbox_text = new_chatbox_text;

  update_chatbox();

}

/**************************************************************************
 ...
**************************************************************************/
function chatbox_scroll_down ()
{
    var scrollDiv;

    if (civclient_state <= C_S_PREPARING) {
      scrollDiv = document.getElementById('pregame_message_area');
    } else {
      scrollDiv = document.getElementById('game_message_area');
    }

    if (scrollDiv != null) {
      var currentHeight = 0;

      if (scrollDiv.scrollHeight > 0) {
        currentHeight = scrollDiv.scrollHeight;
      } else if (scrollDiv.offsetHeight > 0) {
        currentHeight = scrollDiv.offsetHeight;
      }

      scrollDiv.scrollTop = currentHeight;

      previous_scroll = currentHeight;
    }

}

