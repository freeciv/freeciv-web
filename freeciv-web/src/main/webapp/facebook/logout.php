<?php

  /*
   * Logs the user out of the app. Does NOT delete the session or disconnect the user.
   * Basically, this just deletes the cookie.
   *
   */

define(MAIN_PATH, realpath('.'));
include_once MAIN_PATH.'/init.php';

$user = User::getLoggedIn();
if ($user) {
  $user->logOut();
}
go_home();
