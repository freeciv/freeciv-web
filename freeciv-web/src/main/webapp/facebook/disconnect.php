<?php

define(MAIN_PATH, realpath('.'));
include_once MAIN_PATH.'/init.php';

echo render_header();

$user = User::getLoggedIn();

if (!$user) {
  go_home();
}

$params = parse_http_args($_POST, array('save'));

if ($params['save']) {
  if ($user->disconnectFromFacebook()) {
    header('Location: account.php');
  } else {
    $error = 'Could not disconnect from Facebook for an unknown reason.';
  }
}

if ($error) {
  echo '<div class="error">'.$error.'</div>';
}

?>

<h3>Are you sure you want to disconnect your Facebook account?</h3>

<p>
Your Facebook information and friend connections will be deleted. Any information you entered on this site will remain.
</p>

<form action="disconnect.php" method="post">
  <input type="hidden" name="save" value="1" />
  <input type="submit" class="inputsubmit" value="Disconnect From Facebook" />
</form>

<?php
echo render_footer();
