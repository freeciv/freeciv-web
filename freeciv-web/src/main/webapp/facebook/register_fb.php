<?php

define(MAIN_PATH, realpath('.'));
include_once MAIN_PATH.'/init.php';

echo render_header();

 if (!empty($_POST)) {
  $params = parse_http_args($_POST, array('save', 'username'));

  if ($params['save']) {
    $username = mysql_real_escape_string($params['username']);

    $user_validation_errors = User::validateNewFacebookUser($username);

    if ($user_validation_errors != "") {
      echo '<div class="error">'.$user_validation_errors.'</div>';

    } else  {
      $fb_uid = $_COOKIE[FBID_COOKIE_NAME];
      if (!empty($fb_uid)) {
        $user = User::createFromFacebookUID($fb_uid, $username);
      }

    }
  }
}

if ($user && $user_validation_errors == "") {
  $user->logIn();
  go_home();
}

if ($error) {
  echo '<div class="error">'.$error.'</div>';
}

?>


<!-- Eventually do CSSification -->
<div class="register">
<div class="login_sector_fb">
<h2>Register</h2>
Please choose a username for Freeciv.net, which is the nickname that other players
will know you by:<br/><br />

<form action="register_fb.php" method="post">
<table>
  <tr>
    <td>
      <label id="label_username" for="username">Username:</input>
    </td>
    <td>
      <input id="username" class="inputtext" type="text" size="20" value="" name="username"/>
    </td>
  </tr>
 </table>
<input type="hidden" name="save" value="1">
<input type="submit" class="inputsubmit" value="Register" style="margin-left: 80px">
</form>
</div>
</div>
<?php

echo render_footer();
