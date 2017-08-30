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
var chatbox_text = [];
var chatbox_text_dirty = false;
var previous_scroll = 0;
var chatbox_panel_toggle = false;
var longturn_chat_enabled = false;

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
    if (text == null) return;
    if (!check_text_with_banlist(text)) return;
    if (is_longturn()) {
      if (text.indexOf("waiting on") != -1 || text.indexOf("Lost connection") != -1 || text.indexOf("Not enough") != -1 || text.indexOf("has been removed") != -1 || text.indexOf("has connected") != -1) return;
      if (!longturn_chat_enabled) {
        // disable chat in LongTurn games.
        var filtered_text = text.replace(/ *\([^)]*\) */g, "");  // remove text in parenthesis ().
        if (filtered_text.indexOf(":") != -1
         && filtered_text.indexOf("Year: ") !== 0
         && filtered_text.match(/^<font [^>]*>\/help: /) == null
         && filtered_text.match(/^<font [^>]*>\/show: /) == null) {
          return;
        }
      }
    }
    if (text.indexOf("Year:") != -1) text = "<hr style='border-color: #555555;'>" + text;

    if (civclient_state <= C_S_PREPARING) {
      text = text.replace(/#FFFFFF/g, '#000000');
    } else {
      text = text.replace(/#0000FF/g, '#5555FF')
                 .replace(/#006400/g, '#00AA00')
                 .replace(/#551166/g, '#AA88FF')
                 .replace(/#A020F0/g, '#F020FF');
    }

    chatbox_text.push(text);
    chatbox_text_dirty = true;

}

/**************************************************************************
 Returns the chatbox messages.
**************************************************************************/
function get_chatbox_text()
{
  return get_chatbox_msg_list().textContent;
}

/**************************************************************************
 Returns the chatbox message list element.
**************************************************************************/
function get_chatbox_msg_list()
{
  return document.getElementById(civclient_state <= C_S_PREPARING ?
    'pregame_message_area' : 'game_message_area');
}

/**************************************************************************
 Clears the chatbox.
**************************************************************************/
function clear_chatbox()
{
  chatbox_text = [];
  chatbox_clip_messages(0);
}

/**************************************************************************
 This is called at regular time intervals to update the chatbox text window
 with new messages. It is updated at interals for performance reasons
 when many messages appear.
**************************************************************************/
function update_chatbox()
{
  var scrollDiv = get_chatbox_msg_list();

  if (chatbox_text_dirty && scrollDiv != null) {
      for (var i = 0; i < chatbox_text.length; i++) {
        var item = document.createElement('li');
        item.innerHTML = chatbox_text[i];
        scrollDiv.appendChild(item);
      }
      chatbox_text = [];
      chatbox_text_dirty = false;

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
  }

}

/**************************************************************************
 Clips the chatbox text to a maximum number of lines.
**************************************************************************/
function chatbox_clip_messages(lines)
{
  if (lines === undefined) {
    lines = 24;
  }

  var msglist = get_chatbox_msg_list();
  var queued = chatbox_text.length;
  if (queued > lines) {
    chatbox_text.splice(0, queued - lines);
    while (msglist.firstChild) {
      msglist.removeChild(msglist.firstChild);
    }
  } else {
    var remove = msglist.children.length - (lines - queued);
    while (remove-- > 0) {
      msglist.removeChild(msglist.firstChild);
    }
  }

  chatbox_text_dirty = true;
}

/**************************************************************************
 ...
**************************************************************************/
function chatbox_scroll_down ()
{
    var scrollDiv = get_chatbox_msg_list();

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

/**************************************************************************
  Waits for the specified text to appear in the chat log, then
  executes the given JavaScript code.
**************************************************************************/
function wait_for_text(text, runnable)
{
  var chatbox_text = get_chatbox_text();
  if (chatbox_text != null && chatbox_text.indexOf(text) != -1) {
    runnable();
  } else {
    setTimeout(function () {
      wait_for_text(text, runnable);
    }, 100);
  }

}

