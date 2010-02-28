<?php

/*
 * Form handler for the login submission.
 */

define(MAIN_PATH, realpath('.'));
include_once MAIN_PATH.'/init.php';

if (User::getLoggedIn()) {
  go_home();
}
$error = '';

$params = parse_http_args($_POST, array('password', 'username'));

/*
 * If user entered normal username/password,
 * then log them in via their normal account.
 */
if ($params['password'] && $params['username']) {
  $user = User::getByUsername(mysql_real_escape_string($params['username']));

  if (!$user) {
    $error = 'Unknown username: <b>'.$params['username'].'</b>';
  }
  else if (!$user->logIn(md5($params['password']))) {
    $error = 'Bad password for <b>'.$params['username'].'</b>.';
  } else {
    // log in success!
    go_home();
  }
}


echo render_header();
if ($error) {
  echo '<div class="error">'.$error.'</div>';
}

/*
 * Form displayed if something went wrong
 */
?>
<a href="register.php">Register a new account now!</a> Or try again below:
<form action="login.php" method="post">

<div id="loginbox">
<table>
 <tr>
  <td>
    <label id="label_username" for="username">Username:</label>
  </td><td>
    <input id="username" class="inputtext" type="text" size="20" value="<?php echo $params['username']; ?>" name="username"/>
  </td>
 </tr>
 <tr>
  <td>
    <label id="label_password" for="password">Password:</label>
  </td><td>
<input id="password" class="inputtext" type="password" size="20" name="password"/>
  </td>
 </tr>
</table>

<input class="inputsubmit" type="submit" onclick="this.disabled=true;" name="doquicklogin" value="Login"/>

</div>
<?php
  echo render_footer();
?>
