<?php

define(MAIN_PATH, realpath('.'));
include_once MAIN_PATH.'/init.php';

echo render_header();

$usr = " ";
$user = User::getLoggedIn();

if (!$user) {
  echo render_footer();
  exit;
} else {
  $usr = $user->username;
}


$furl = "Join me in a Freeciv.net multiplayer game at http://www.freeciv.net/games/" . $usr;
register_feed_form_js($furl);
echo "<h2>Invite friends from Facebook</h2>";
echo "Invite your friends to play, then <a href='/civclientlauncher?action=new'>start a new game</a>.";
echo '</div>';
echo render_footer();
