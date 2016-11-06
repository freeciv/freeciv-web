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

var opponent = null;
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
    + "can challenge another player, and each player will be notified when "
    + "it is their turn to play through e-mail. If you are a new player, then click the sign up button below. These are the game rules:<br>" 
    + "<ul><li>The game will have two human players playing alternating turns. Each player will get an e-mail when it is their turn to play.</li>" 
    + "<li>Standard Freeciv-web rules are used with some changes to map size, research speed, start units and gold to speed up games.</li>"  
    + "<li>Please complete your turn as soon as possible, and use at no longer than 7 days until you complete your turn.</li>"
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
                                    create_new_pbem_user();
				},
				"Log In" : function() {
                                    login_pbem_user();
				},
				  "Account...": function() {
                                    close_pbem_account();
				}, 
				  "Forgot password?": function() {
                                    forgot_pbem_password();
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


}

/**************************************************************************
...
**************************************************************************/
function login_pbem_user() 
{

  var title = "Log in";
  var message = "Log in to your Freeciv-web user account:<br><br>"
                + "<table><tr><td>Username:</td><td><input id='username' type='text' size='25' maxlength='30' onkeyup='return forceLower(this);'></td></tr>"  
                + "<tr><td>Password:</td><td><input id='password' type='password' size='25'></td></tr></table><br><br>"
                + "<div id='username_validation_result'></div>";   

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

}



/**************************************************************************
...
**************************************************************************/
function create_new_pbem_user() 
{

  var title = "New user account";
  var message = "Create a new Freeciv-web user account with information about yourself:<br><br>"
                + "<table><tr><td>Username:</td><td><input id='username' type='text' size='25' maxlength='30' onkeyup='return forceLower(this);'></td></tr>"  
                + "<tr><td>Email:</td><td><input id='email' type='email' size='25' maxlength='64' ></td></tr>"  
                + "<tr><td>Password:</td><td><input id='password' type='password' size='25'></td></tr>"
                + "<tr><td>Confirm password:</td><td><input id='confirm_password' type='password' size='25'></td></tr></table><br>"
                + "<div id='username_validation_result'></div><br>"
                + "<div id='captcha_element'></div><br><br>"
                + "<div><small><ul><li>It is free and safe to create a new account on Freeciv-web.</li>"
                + "<li>Other players can use your username to start Play-by-email games with you.</li>"
                + "<li>You will not receive any spam and your e-mail address will be kept safe.</li>"
                + "<li>You can cancel your account at any time if you want.</li>"
                + "</ul></small></div>";   

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  $("#dialog").html(message);
  $("#dialog").attr("title", title);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "90%" : "60%",
			buttons:
			{
                                "Cancel" : function() {
	                          show_pbem_dialog();
				},
				"Signup new user" : function() {
                                  create_new_freeciv_user_account_request("pbem");
				}
			}
		});

  $("#dialog").dialog('open');
  grecaptcha.render('captcha_element', {
          'sitekey' : '6LfpcgMTAAAAAPRAOqYy6ZUhuX6bOJ7-7-_1V0FL'
        });

  $("#username").blur(function() {
   $.ajax({
     type: 'POST',
     url: "/validate_user?userstring=" + $("#username").val(),
     success: function(data, textStatus, request) {
        if (data != "user_does_not_exist") {
          $("#username_validation_result").html("The username is already taken. Please choose another username.");
        } else {
          $("#email").blur(function() {
          $.ajax({
            type: 'POST',
            url: "/validate_user?userstring=" + $("#email").val(),
            success: function(data, textStatus, request) {
               if (data != "user_does_not_exist") {
                 $("#username_validation_result").html("The e-mail is already registered. Please choose another.");
               } else {
                 $("#username_validation_result").html("");
               }
             }
           });
         });
        }
      }
    });
  });

}


/**************************************************************************
...
**************************************************************************/
function login_pbem_user_request() 
{

  username = $("#username").val().trim();
  var password = $("#password").val().trim();

  $.ajax({
   type: 'POST',
   url: "/login_user?username=" + encodeURIComponent(username) + "&password=" + encodeURIComponent(password),
   success: function(data, textStatus, request){
       if (data != null && data == "OK") {
         simpleStorage.set("username", username);
         simpleStorage.set("password", password);
         if ($.getUrlVar('savegame') != null) {
           handle_pbem_load();
         } else {
           challenge_pbem_player_dialog();
         }
       } else {
         $("#username_validation_result").html("Incorrect username or password. Please try again!");
       }
     },
   error: function (request, textStatus, errorThrown) {
     swal("Login user failed. ");
   }
  });
}


/**************************************************************************
 Shows the Freeciv play-by-email dialog.
**************************************************************************/
function challenge_pbem_player_dialog() 
{

  var title = "Find other players to play with!";
  var message = "Enter the username or e-mail address of the other player: "
   + "<table><tr><td>Username/E-Mail:</td><td><input id='opponent' type='text' size='25' maxlength='64'"
   + " onkeyup='return forceLower(this);'></td></tr>"
   + "</table><br>"
   + "If the other player already has an account, then you will play the first turn. If you enter the e-mail " 
   + "address of a new player, then we will send an invitation e-mail to that player " 
   + "and the other player will play the first turn."
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
				$("#opponent").val(data)
			      }  });
			},
			"Start game": function() {
                                    create_new_pbem_game($("#opponent").val().trim());
			 }
			}

		});

  var invited = $.getUrlVar('invited_by');
  if (invited != null) {
    $("#opponent").val(invited);
    $(".ui-dialog-buttonset button").first().hide();
  }

  $("#dialog").dialog('open');
}

/**************************************************************************
 Determines if the email is valid
**************************************************************************/
function validateEmail(email) {
    var re = /^([\w-]+(?:\.[\w-]+)*)@((?:[\w-]+\.)*\w[\w-]{0,66})\.([a-z]{2,6}(?:\.[a-z]{2})?)$/i;
    return re.test(email);
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
function create_new_pbem_game(check_opponent) 
{

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
        opponent = data;
        network_init();
        $("#dialog").dialog('close');
        setTimeout("create_pbem_players();", 3500);
        show_dialog_message("Game ready", "Play-By-Email game is now ready to start. " +
        "Click the start game button to play the first turn. You can also configure some " +
        "game settings before the game begins. The default settings are recommended. " +
        "Some settings are not supported in PBEM games, " +
        "such as more than two players, AI players or diplomacy. " +
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
  if (opponent != null) {
    send_message("/create " + opponent);
    setTimeout("pbem_init_game();", 1200);
  } else {
    swal("Error: invalid opponent selected.");
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
      send_message("/ai " + opponent);
    }
  }
}

/**************************************************************************
...
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
  loadTimerIdA = setTimeout("load_game_real('" + savegame + "');", 1500);
  loadTimerIdB = setTimeout("activate_pbem_player();", 2500);

 
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
  send_message_delayed("/take " + username, 100);
  send_message_delayed("/start", 200);
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
                + "<div id='username_validation_result'></div>";   

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  $("#dialog").html(message);
  $("#dialog").attr("title", title);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "80%" : "60%",
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
 Send password link to user. TODO: This method is not complete yet.
**************************************************************************/
function forgot_pbem_password() 
{

  var title = "Forgot your password?";
  var message = "Please enter your e-mail address to get your password:<br><br>"
                + "<table><tr><td>E-mail address:</td><td><input id='email' type='text' size='25'></td></tr>"  
                + "</table><br><br>"
                + "<div id='username_validation_result'></div>";   

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  $("#dialog").html(message);
  $("#dialog").attr("title", title);
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "80%" : "60%",
			buttons:
			{ 
                                "Cancel" : function() {
	                          show_pbem_dialog();
				},
				"Send password" : function() {
                                  /* TODO. */
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

  $.ajax({
   type: 'POST',
   url: "/deactivate_user?username=" + usr + "&password=" + password ,
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
