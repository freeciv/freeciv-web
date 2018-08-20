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

var opponents = [];
var pbem_phase_ended = false;

var invited_players = [];

/**************************************************************************
 Shows the Freeciv play-by-email dialog.
**************************************************************************/
function show_pbem_dialog() 
{
  var title = "Welcome to Freeciv-web";
  var message = "";

  if ($.getUrlVar('invited_by') != null) {
    var invited = $.getUrlVar('invited_by').replace(/[^a-zA-Z]/g,'');
    message = "You have been invited by " + invited + " for a Play-by-Email game of Freeciv-web. "
    + "You and " + invited + " will play alternating turns, and you will get an e-mail every time "
    + "it is your turn to play. First you can create a new user or log-in, then you will play "
    + "the first turn.";
  } else if ($.getUrlVar('savegame') != null) {
    message = "It is now your turn to play this Play-by-Email game. Please login to play your turn.";
    if (pbem_duplicate_turn_play_check()) return;
  
  } else {
    message = "You are about to start a Play-by-Email game, where you "
    + "can challenge other players, and each player will be notified when "
    + "it is their turn to play through e-mail. If you are a new player, then click the sign up button below. These are the game rules:<br>" 
    + "<ul><li>The game will have between 2 and 4 human players playing alternating turns. Each player will get an e-mail when it is their turn to play.</li>"
    + "<li>Standard Freeciv-web rules are used with some changes to map size, research speed, start units and gold to speed up games.</li>"  
    + "<li>Please complete your turn as soon as possible, and use at no longer than 7 days until you complete your turn.</li>"
    + "<li>Results of games with 2 players are stored to rank players.</li>"
    + "<li>Please post feedback and arrange new games on the <a href='http://forum.freeciv.org/f/viewforum.php?f=24' style='color: black;' target='_new'>forum</a>.</li>"
    + "<li id='user_count'></li></ul>"; 
  }

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");
  $("#dialog").html(message);
  $("#dialog").attr("title", title);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "95%" : "60%",
			buttons:
			{
				"Signup new user": function() {
                                    show_new_user_account_dialog("pbem");
				},
				"Log In" : function() {
                                    login_pbem_user();
				},
				  "Account...": function() {
                                    close_pbem_account();
				}


			}

		});

  $("#dialog").dialog('open');

  $.ajax({
   type: 'POST',
   url: "/user_count" ,
   success: function(data, textStatus, request){
	$("#user_count").html("We are now " + data + " players available for Play-By-Email games.");
   }  });

  var stored_username = simpleStorage.get("username", "");
  var stored_password = simpleStorage.get("password", "");
  if (stored_username != null && stored_username != false && stored_password != null && stored_password != false) {
    // Not allowed to create a new user account when already logged in.
    $(".ui-dialog-buttonset button").first().button("disable");
  }

}

/**************************************************************************
...
**************************************************************************/
function login_pbem_user() 
{

  var title = "Log in";
  var message = "Log in to your Freeciv-web user account:<br><br>"
                + "<table><tr><td>Username:</td><td><input id='username' type='text' size='25' maxlength='30' onkeyup='return forceLower(this);'></td></tr>"  
                + "<tr><td>Password:</td><td><input id='password' type='password' size='25'> &nbsp; <a class='pwd_reset' href='#' style='color: #666666;'>Forgot password?</a></td></tr></table><br><br>"
                + "<div id='username_validation_result' style='display:none;'></div><br><br>";

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  $("#dialog").html(message);
  $("#dialog").attr("title", title);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "80%" : "40%",
			buttons:
			{
                                "Cancel" : function() {
	                          show_pbem_dialog();
				},
				"Login" : function() {
                                  login_pbem_user_request();
				}
			}
		});

  var stored_username = simpleStorage.get("username", "");
  if (stored_username != null && stored_username != false) {
    $("#username").val(stored_username);
  }
  var stored_password = simpleStorage.get("password", "");
  if (stored_password != null && stored_password != false) {
    $("#password").val(stored_password);
  }

  $("#dialog").dialog('open');

  $('#dialog').keyup(function(e) {
    if (e.keyCode == 13) {
      login_pbem_user_request();
    }
  });

  $(".pwd_reset").click(forgot_pbem_password);

}

/**************************************************************************
...
**************************************************************************/
function login_pbem_user_request() 
{

  username = $("#username").val().trim();
  var password = $("#password").val().trim();
  if (password != null && password.length > 2) {
    var shaObj = new jsSHA("SHA-512", "TEXT");
    shaObj.update(password);
    var sha_password = encodeURIComponent(shaObj.getHash("HEX"));

    $.ajax({
     type: 'POST',
     url: "/login_user?username=" + encodeURIComponent(username) + "&sha_password=" + sha_password,
     success: function(data, textStatus, request){
         if (data != null && data == "OK") {
           simpleStorage.set("username", username);
           simpleStorage.set("password", password);
           if ($.getUrlVar('savegame') != null) {
             handle_pbem_load();
           } else {
             challenge_pbem_player_dialog(null);
           }
         } else {
           $("#username_validation_result").html("Incorrect username or password. Please try again!");
           $("#username_validation_result").show();
         }
       },
     error: function (request, textStatus, errorThrown) {
       swal("Login user failed. ");
     }
    });
  } else {
    swal("Invalid password");
  }
}


/**************************************************************************
 Shows the Freeciv play-by-email dialog.
**************************************************************************/
function challenge_pbem_player_dialog(extra_intro_message)
{

  var title = "Find other players to play with!";
  var message = "";
  if (extra_intro_message != null) message = extra_intro_message += "<br>";

  message += "Total number of human players: " +
  "<select name='playercount' id='playercount'>" +
            "<option value='2'>2</option>" +
            "<option value='3'>3</option>" +
            "<option value='4'>4</option>" +
      "</select> <br><br>" +
      "<div id='opponent_box'></div>";

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");
  $("#dialog").html(message);
  $("#dialog").attr("title", title);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "80%" : "40%",
			buttons: {"Invite random player": function() {
			  $.ajax({
			   type: 'POST',
			   url: "/random_user" ,
			   success: function(data, textStatus, request){
                var playercount = parseFloat($('#playercount').val());
                if (playercount == 2) {
				  $("#opponent").val(data)
				} else {
				  $("#opponent_1").val(data)
				}
              }
              });
			},
			"Cancel" : function() {
               show_pbem_dialog();
            },

			"Start game": function() {
               create_new_pbem_game();
			 }
			}

		});

  var invited = $.getUrlVar('invited_by');
  if (invited != null) {
    $("#opponent").val(invited);
    $(".ui-dialog-buttonset button").first().hide();
  }
  prepare_pbem_opponent_selection();
  $('#playercount').change(prepare_pbem_opponent_selection);

  $("#dialog").dialog('open');
}

/**************************************************************************
 TODO: validate all usernames when game has 3 or more players.
**************************************************************************/
function prepare_pbem_opponent_selection() {
    var playercount = parseFloat($('#playercount').val());
    opponents = [];
    if (playercount == 2) {
      var msg = "Enter the username or e-mail address of the other player: "
       + "<table><tr><td>Username/E-Mail:</td><td><input id='opponent' type='text' size='25' maxlength='64'"
       + " onkeyup='return forceLower(this);'></td></tr>"
       + "</table><br>"
       + "If the other player already has an account, then you will play the first turn.<br>If you enter the e-mail "
       + "address of a new player, then we will send an invitation e-mail to that player "
       + "and the other player will play the first turn."
       $("#opponent_box").html(msg);

    } else if (playercount == 3 || playercount == 4) {
      var msg = "Enter the <u>usernames</u> of the other <u>human</u> players: "
       + "<table>"
       + "<tr><td>You:</td><td>" + username + "</td></tr>"
       + "<tr><td>Username 2:</td><td><input id='opponent_1' type='text' size='25' maxlength='64'"
       + " onkeyup='return forceLower(this);'></td></tr>"
       + "<tr><td>Username 3:</td><td><input id='opponent_2' type='text' size='25' maxlength='64'"
       + " onkeyup='return forceLower(this);'></td></tr>";
      if (playercount == 4) {
        msg += "<tr><td>Username 4:</td><td><input id='opponent_3' type='text' size='25' maxlength='64'"
                      + " onkeyup='return forceLower(this);'></td></tr>";
      }

       msg += "</table><br>"
       + "Before you fill in this form, the other players have to create their account and tell you their usernames. <br>" +
       "You will play the first turn. The other players will get an e-mail when it is their turn to play.<br>" +
       "This type of game can have human players only."
       $("#opponent_box").html(msg);

    }
  }

/**************************************************************************
 Determines if the email is valid
**************************************************************************/
function validateEmail(email) {
    var checkemail = email;
    if (checkemail != null) checkemail = checkemail.replace("+", "");  // + is allowed.
    var re = /^([\w-]+(?:\.[\w-]+)*)@((?:[\w-]+\.)*\w[\w-]{0,66})\.([a-z]{2,6}(?:\.[a-z]{2})?)$/i;
    return re.test(checkemail);
}

/**************************************************************************
...
**************************************************************************/
function forceLower(strInput) 
{
  strInput.value=strInput.value.toLowerCase();
}

/**************************************************************************
...
**************************************************************************/
function create_new_pbem_game()
{
  opponents = [];
  var playercount = parseFloat($('#playercount').val());

  if (playercount == 2) {
    var check_opponent = $("#opponent").val();
    if (check_opponent != null) check_opponent = check_opponent.trim();

    if (invited_players.indexOf(check_opponent) != -1) {
      swal("You have already invited " + check_opponent + ". There is no "
          + "need to invite multiple times. It can take some time before "
          + "the invitation email arrives to " + check_opponent);
      return;
    }

    if (check_opponent == username) {
      swal("No playing with yourself please.");
      return;
    }

    invited_players.push(check_opponent);

    $.ajax({
     type: 'POST',
     url: "/validate_user?userstring=" + check_opponent + "&invited_by=" + username,
     success: function(data, textStatus, request){
        if (data == "invitation") {
          send_pbem_invitation(check_opponent);

        } else if (data == "user_does_not_exist") {
          swal("Opponent does not exist. Please try another username.");

        } else if (data != null && data.length > 3) {
          opponents.push(data);
          network_init();
          $("#dialog").dialog('close');
          setTimeout(create_pbem_players, 3500);
          show_dialog_message("Game ready", "Play-By-Email game is now ready to start. " +
          "Click the start game button to play the first turn. You can also configure some " +
          "game settings before the game begins. The default settings are recommended. " +
          "Some settings are not supported in PBEM games, " +
          "such as more than four players, AI players or diplomacy. " +
          "As the first player, you can choose nation for both players. " +
          "Have a fun Play-By-Email game!"
          );


        } else {
          swal("Problem starting new pbem game.");
        }
      },
     error: function (request, textStatus, errorThrown) {
       swal("Opponent does not exist. Please try another username.");
     }
    });

  } else {
    // 3 or 4 players.
    var pbem_ready_players = 1;
    var game_start_ready = true;
    opponents.push($("#opponent_1").val());
    opponents.push($("#opponent_2").val());
    if (playercount == 4) {
      opponents.push($("#opponent_3").val());
    }

    for (var i = 0; i < opponents.length; i++) {
      if (opponents[i].length < 1) {
        swal("Please fill inn all players names.");
        return;
      } else if (opponents[i].indexOf("@") != -1) {
        swal("Please specify the username of the other player, not their e-mail address.");
        return;
      } else {
       $.ajax({
         async: false,
         type: 'POST',
         url: "/validate_user?userstring=" + opponents[i] + "&invited_by=" + username,
         success: function(data, textStatus, request) {
           if (data == "user_does_not_exist") {
             swal("Opponent does not exist. Please try another username.");
             game_start_ready = false;
           } else {
             pbem_ready_players = pbem_ready_players + 1;
           }
         }
        });
      }
    }

    // start new pbem game
    if (game_start_ready && pbem_ready_players == playercount) {
      network_init();
      $("#dialog").dialog('close');
      setTimeout(create_pbem_players, 3500);
      show_dialog_message("Game ready", "Play-By-Email game is now ready to start. " +
          "Click the start game button to play the first turn. You can also configure some " +
          "game settings before the game begins. The default settings are recommended. " +
          "Some settings are not supported in PBEM games, " +
          "such as more than four players, AI players or diplomacy. " +
          "As the first player, you can choose nation for all players. " +
          "Have a fun Play-By-Email game!"
          );
    }
  }
}

/**************************************************************************
...
**************************************************************************/
function send_pbem_invitation(email) 
{
  $.ajax({
   type: 'POST',
   url: "/mailstatus?action=invite&to=" + email + "&from=" + username,
   success: function(data, textStatus, request){
       swal(email + " has been invited to Freeciv-web. You will "
             + "receive an e-mail when it is your turn to play. Now "  
             + "you can wait for the other player.");
       $("#opponent").val("")
      },
   error: function (request, textStatus, errorThrown) {
     swal("Error: Unable to invite the opponent.");
   }
  });


}

/**************************************************************************
...
**************************************************************************/
function create_pbem_players()
{
  if (ws != null && ws.readyState === 1) {
    if (opponents != null && opponents.length > 0) {
      for (var i = 0; i < opponents.length; i++) {
        send_message("/create " + opponents[i]);
      }
      setTimeout(pbem_init_game, 1200);

    } else {
      swal("Error: invalid opponent selected.");
    }
  } else {
    setTimeout(create_pbem_players, 500);
  }
}

/**************************************************************************
...
**************************************************************************/
function set_human_pbem_players()
{
  for (var player_id in players) {
    var pplayer = players[player_id];
    if (pplayer['flags'].isSet(PLRF_AI) == true 
        && pplayer['name'].toUpperCase() != username.toUpperCase()) {
      send_message("/aitoggle " + pplayer['name']);
    }
  }
}

/**************************************************************************
 Is this a Play-By-Email game?
**************************************************************************/
function is_pbem() 
{
  return ($.getUrlVar('action') == "pbem");
}

/**************************************************************************
...
**************************************************************************/
function pbem_end_phase() 
{
  if (pbem_phase_ended) return;
  pbem_phase_ended = true;
  send_message("/save");

  show_dialog_message("Play By Email turn over", 
      "Your turn is now over in this Play By Email game. Now the next player " +
      "will get an email with information about how to complete their turn. " +
      "You will also get an email about when it is your turn to play again. " +
      "See you again soon!"  );
  if ($.getUrlVar('savegame') != null) {
    simpleStorage.set("pbem_" + $.getUrlVar('savegame'), "true");
  }
  $(window).unbind('beforeunload');
  setTimeout("window.location.href ='/';", 5000);
}

/**************************************************************************
...
**************************************************************************/
function handle_pbem_load()
{
  network_init();
  var savegame = $.getUrlVar('savegame');
  $("#dialog").dialog('close');
  $.blockUI();

  wait_for_text("You are logged in as", function () {
    load_game_real(savegame);
  });
  wait_for_text("Load complete", activate_pbem_player);

}

/**************************************************************************
 called when starting a new PBEM game.
**************************************************************************/
function pbem_init_game()
{
  set_human_pbem_players();
  $.post("/freeciv_time_played_stats?type=pbem").fail(function() {});
}


/**************************************************************************
 called when loading a PBEM game.
**************************************************************************/
function activate_pbem_player()
{
  send_message("/take " + username);
  send_message_delayed("/start", 50);
}


/**************************************************************************
 Dialog for the user to close their user accounts.
**************************************************************************/
function close_pbem_account() 
{

  var title = "Close account";
  var message = "To deactivate your account, please enter your username and password:<br><br>"
                + "<table><tr><td>Username:</td><td><input id='username' type='text' size='25' onkeyup='return forceLower(this);'></td></tr>"  
                + "<tr><td>Password:</td><td><input id='password' type='password' size='25'></td></tr></table><br><br>"
                + "<div id='username_validation_result' style='display:none;'></div><br><br>";

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  $("#dialog").html(message);
  $("#dialog").attr("title", title);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "80%" : "50%",
			buttons:
			{
				"Cancel" : function() {
	                          show_pbem_dialog();
				},
				"Unsubscribe" : function() {
                                  request_deactivate_account();
				}
			}
		});

  $("#dialog").dialog('open');
}



/**************************************************************************
 Will request the user to be deactivated  (activated='0' in DB).
**************************************************************************/
function request_deactivate_account()
{
  var usr = $("#username").val().trim();
  var password = $("#password").val().trim();

  var shaObj = new jsSHA("SHA-512", "TEXT");
  shaObj.update(password);
  var sha_password = encodeURIComponent(shaObj.getHash("HEX"));

  $.ajax({
   type: 'POST',
   url: "/deactivate_user?username=" + usr + "&sha_password=" + sha_password,
   success: function(data, textStatus, request){
       swal("User account has been deactivated!");

     },
   error: function (request, textStatus, errorThrown) {
     swal("deactivate user failed.");
   }
  });

}

/**************************************************************************
 Checks for user playing same turn twice.
**************************************************************************/
function pbem_duplicate_turn_play_check()
{
  var pbem_savegame = $.getUrlVar('savegame');
  if (pbem_savegame != null) {
    var previously_played = simpleStorage.get("pbem_" + pbem_savegame, "");
    if (previously_played != null) {
      swal("This Play-By-Email turn has already been played and can't be played again. Sorry!");
      return true;
    } else {
      return false;
    }
    
  }

}

/**************************************************************************
...
**************************************************************************/
function get_pbem_game_key()
{
  return "pbem_tech_" + client.conn.username + players[0]['name'] + players[1]['name'];
}
