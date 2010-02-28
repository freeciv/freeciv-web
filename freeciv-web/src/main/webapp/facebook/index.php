<?php

define(MAIN_PATH, realpath('.'));
include_once MAIN_PATH.'/init.php';

if (!empty($_SERVER['HTTP_REFERER'])){
    $referer        = $_SERVER['HTTP_REFERER'];
    if (strstr($referer,'facebook.com')){
         setcookie("facebook_mode", "true", time()+36000, "/");
    }
}
  
 

echo render_header();

$user = User::getLoggedIn();

if (!$user) {
  echo render_logged_out_index();
  echo render_footer();
  exit;
} else {
  freecivAuthUser($user->username);
}




// If the user has connected, then show info about their friends

/*if ($user->is_facebook_user()) {
  echo '<div class="bluebox friends_box">';
  echo '<h3>Friends</h3>';

  $friends = get_connected_friends($user);
  if (is_array($friends) && !empty($friends)) {
    echo render_friends_table($friends);
  } else {
    echo 'You don\'t have any friends yet!<br />';
  }

  echo '<div class="connect_invites">';
  echo render_connect_invite_link( has_existing_friends  (count($friends) > 0));
  echo '</div>';

  echo '</div>';
}*/

echo '</div>';
echo render_footer();
