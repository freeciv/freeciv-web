<?php

include_once 'lib/core.php';
include_once 'lib/fbconnect.php';

/*
 * This is called after the Facebook Connect button is pressed on the front page
 * or anywhere on the site. This is the code that links accounts.
 */

// Gets any credentials for The Run Around
$user = User::getLoggedIn();

if (idx($_POST,'save')) {
  // Facebook client library will validate the signature against the secret
  // key, and produce a user id if it all checks out
  $fb_uid = facebook_client()->get_loggedin_user();

  if ($fb_uid) {
    // The user is connecting an existing account
    // with their Facebook account.
    if ($user && $_POST['link_to_current_user']) {
      $user->connectWithFacebookUID($fb_uid);
    }
    // There is no existing account, and no existing
    // Facebook account. So create a new one.
    else if (!$user) {
      $user = User::createFromFacebookUID($fb_uid);
    }
  }
}


if ($user) {
  $user->logIn();
  echo 1;
} else {
  echo 0;
}
