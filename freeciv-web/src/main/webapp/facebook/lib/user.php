<?php

define('USER_COOKIE_NAME', 'rb_current_user');
define('FBID_COOKIE_NAME', 'freeciv_fbid');
define('MAX_DISPLAY_RUNS', 25); // Maximum number of runs to display

/*
 * This is a pretty generic user object.
 *
 * Logging in / logging out is done by setting a simple
 * cookie with the username. As it is trivial to spoof users in this
 * system, a real production app will need a more secure means of storing
 * state. Connect is intended to fit into an existing login system,
 * so you would use whatever your site already uses for this.
 *
 *
 */

class User {
  public $username;
  public $name;
  public $email;
  public $fb_uid;
  public $password; // should be protected, temp hack
  protected $runs = null;

  /*
   * Figure out who the logged in user is.
   *
   * There are two ways of doing this:
   *
   * 1. Use the site-specific cookie auth system. Every site has one
   *    for managing their users; if someone is logged in with that method, great
   *
   * 2. If the user is logged in with Facebook, then return that associated account.
   *     The site-specific auth doesn't factor in.
   *
   * The philosophy here is that Facebook constitutes a "single-sign-on" experience:
   * once you've connected your account to a site, whenever you visit that site again,
   * it should just automatically know who you are. Therefore, logout logs you out
   * of Facebook, and just being logged into Facebook is sufficient to log you in here.
   *
   * We achieve that through the Javascript API (see facebook_onload() function). The JS
   * retrieves the session and sets some cookies; the PHP client lib verifies those
   * cookies and returns the logged in user. If we are able to get a user, then it means
   * that the user has already authorized the app, so just load their account (or
   * create a new one) and log them in.
   *
   */
  static function getLoggedIn() {

    $native_user = User::getLoggedInNative();
    $fb_uid = facebook_client()->get_loggedin_user();
    if ($fb_uid) {
      $user = User::getByFacebookUID($fb_uid);

      if ($native_user) {
        // connect their accounts.
        // this way the facebook account is their sole means of auth
        // for this session, so a "logout" click will work
        $native_user->logOut();
        $native_user->connectWithFacebookUID($fb_uid);
        $user = $native_user;
      } else if (!$user) {
        setcookie(FBID_COOKIE_NAME, $fb_uid);
        header('Location: register_fb.php');

      }
    }

    if ($native_user && !$user) {
      $user = $native_user;
    }

    return $user;
  }

  /*
   * This is the original getLoggedIn function, before Facebook Connect.
   * Retrieve the current active user based on our custom cookie.
   */
  static function getLoggedInNative() {
    // check the logged-in username first
    $username = $_COOKIE[USER_COOKIE_NAME];
    return ($username && $username != 'unknown') ?
      User::getByUsername($username) : null;
  }

  static function getByUsername($username) {
    static $users_by_username = array();
    if (isset($users_by_username[$username])) {
      return $users_by_username[$username];
    }

    $ret = queryf("SELECT * FROM auth WHERE username = %s", $username);

    if (!$ret) {
      error_log("Could not find username ". $username);
      return null;
    }

    $row = mysql_fetch_assoc($ret);

    if (!$row) {
      error_log("Could not find username ". $username);
      return null;
    }

    return $users_by_username[$username] = new User($row);
  }

  /*
   * Look up a user by their Facebook user ID.
   *
   * @return null on failure or if not found, User otherwise
   */
  static function getByFacebookUID($fb_uid) {
    if (!$fb_uid) {
      return null;
    }

    $ret = queryf("SELECT * FROM auth WHERE fb_uid = %s", $fb_uid);

    if (!$ret) {
      error_log('Could not fetch from db for fb_uid ' . $fb_uid);
      return null;
    }

    $row = mysql_fetch_assoc($ret);

    if ($row) {
      return new User($row);
    }
  }

 
  /*
   * Create an account for the user based on Facebook UID.
   *
   * Note that none of the Facebook data is actually stored in the DB - basically
   * we store only the UID and generate all other info (name, pic, etc) on the fly.
   */
  static function createFromFacebookUID($fb_uid, $username) {

    //  create a new one user
    $user_params = array();
    $user_params['fb_uid'] = $fb_uid;

    $user_params['username'] = $username;

    // Don't add a password for the user. The whole point is that they log in
    // via Facebook. You could choose to let them add a password if you want
    // to give them an alternate means of login, but it's not recommended.
    $user_params['password'] = '';

    $user = new User($user_params);
    $user->name = $user->getName();

    // write to the db
    if (!$user->save()) {
      return null;
    }

    return $user;
  }

  function __construct($data) {
    $this->username = idx($data,'username',null);
    $this->name = idx($data,'name','');
    $this->email = idx($data,'email','');
    $this->password = idx($data,'password','');
    $this->fb_uid = idx($data,'fb_uid',0);

    if ($this->username == 'unknown') {
      throw new Exception("Cannot create a user with name 'unknown'");
    }
  }

  function is_facebook_user() {
    return ($this->fb_uid > 0);
  }

  function getName() {
    // override the user's name if it is a facebook user, regardless
    if ($this->is_facebook_user()) {
      $info = facebook_get_fields($this->fb_uid, array('name'));
      if (!empty($info)) {
        return $info['name'];
      }
    }
    return $this->name;
  }

  function getEmail() {
    // user can override themselves if they want to
    return ($this->is_facebook_user() && !$this->email) ?
      ''
      : $this->email;
  }

  function getFacebookBadge() {
    return ($this->is_facebook_user()) ?
      '<img src="http://static.ak.fbcdn.net/images/icons/favicon.gif" />'
      : '&nbsp;';
  }

  function getProfilePic($show_logo=false) {
    return ($this->is_facebook_user()) ?
      ('<fb:profile-pic uid="'.$this->fb_uid.'" size="square" ' . ($show_logo ? ' facebook-logo="true"' : '') . '></fb:profile-pic>')
      : '<img src="http://static.ak.fbcdn.net/pics/q_default.gif" />';
  }

  function getStatus() {
    return ($this->is_facebook_user()) ?
      '<fb:user-status uid="'.$this->fb_uid.'" ></fb:user-status>'
      : '';
  }

  /*
   * Logging in and out is accomplished by simply setting a cookie. In a real
   * production application, this would probably have an additional layer of
   * security to protect from spoofing users, but in the sample app it's not
   * necessary.
   *
   */
  function logIn($password = null) {

    // facebook users are already logged in, no need for cookie
    if ($this->is_facebook_user() &&
        facebook_client()->get_loggedin_user()) {
      return false;
    }

    // In a real app, you would probably store the password hash,
    // not the raw password. But since this is a demo app, just store
    // natively for security
    if ($password !== null &&
        $this->password != $password) {
      error_log("Attempt to log in for user "
                .$user->username
                ." with password '"
                . $password
                . "' . Real password is '"
                .$this->password
                ."'");
      return false;
    }
    setcookie(USER_COOKIE_NAME, $this->username);
    return true;
  }

  /*
   * Deletes the login cookie.
   *
   */
  function logOut() {
    setcookie(USER_COOKIE_NAME, 'unknown');
    if ($this->is_facebook_user()) {
      try {
        // it's important to expire the session, or else the user
        // may find themselves logged back in when they come back
        // This is NOT the same as disconnecting from the app - you
        // can get a new session at any time with auth.getSession.
        facebook_client()->expire_session();
      } catch (Exception $e) {
        // nothing, probably an expired session
      }
    }
  }

  /*
   * Associate this user account with the given Facebook user.
   *
   */
  function connectWithFacebookUID($fb_uid) {
    // if there is already a facebook account
    if ($fb_user = User::getByFacebookUID($fb_uid)) {
      // if there are two separate accounts for the same
      // user, then delete the Facebook-specific one
      // and connect the facebook id with the existing
      // account.
      //
      // a real site wouldn't actually delete an account -
      // the user should probably control the merging
      // of data from the to-be-deleted account to their own
      if ($fb_user->username != $this->username) {
        $fb_user->delete();
      }
    }

   if (!$fb_uid) {
      return false;
   }
   $this->fb_uid = $fb_uid;
   return $this->save();
  }

  /*
   * Restore the account to its original self, removing all Facebook info.
   */
  function disconnectFromFacebook() {
    if ($this->email) {
      facebook_unregisterUsers(array(email_get_public_hash($this->email)));
    }

    $this->fb_uid = 0;
    return $this->save();
  }

  function save() {
    $email_hash = email_get_public_hash($this->email);
    // if there's already a row out there
    $ret = queryf("INSERT INTO auth (username, name, password, email, fb_uid, email_hash) VALUES (%s, %s, %s, %s, %d, %s) "
                  ."ON DUPLICATE KEY UPDATE name = %s, password = %s, email = %s, fb_uid = %s, email_hash = %s",
                  $this->username,
                  $this->name, $this->password, $this->email, (int) 0 + $this->fb_uid, $email_hash,
                  $this->name, $this->password, $this->email, (int) 0 + $this->fb_uid, $email_hash);

    return (bool) $ret;
  }


  /* Validates the input of the user registration.
   *
   */

  function validateNewUser() {
    $errors = "";

    /* 1. Detect empty fields. */
    if (empty($this->username) || empty($this->email) || empty($this->password) || empty($this->name)) {
      return "Please complete all fields in the form. Some required information is missing.";
    }

    /* 2. Check for an available username. */
    $ret = queryf("SELECT count(*) as count FROM auth WHERE username = %s", $this->username);

    if (!$ret) {
      return "db error in fetch";
    }

    $row = mysql_fetch_assoc($ret);
    if ($row) {
      $my_count = $row['count'];
      if ($my_count != 0) {
        $errors .= "The username is not available. Please choose another username. <br>";
      }
    }

    /* 3. Check for an available email address. */
    $ret = queryf("SELECT count(*) as count FROM auth WHERE email = %s", $this->email);

    if (!$ret) {
      return "db error in fetch";
    }

    $row = mysql_fetch_assoc($ret);
    if ($row) {
      $my_count = $row['count'];
      if ($my_count != 0) {
        $errors .= "The email is already registered. <br>"; 
      }
    }

   /* 4. Username must be alphanumeric.  */

   if (!ctype_alnum($this->username)) {
     $errors .= "The username contains invalid characters<br>";
   }


    return $errors;
  }

  /* Validates the input of the user registration.
   *
   */

  function validateNewFacebookUser($user_name) {
    $errors = "";

    /* 1. Detect empty fields. */
    if (empty($user_name)) {
      return "The username can't be empty";
    }

    /* 2. Check for an available username. */
    $ret = queryf("SELECT count(*) as count FROM auth WHERE username = %s", $user_name);

    if (!$ret) {
      return "db error in fetch";
    }

    $row = mysql_fetch_assoc($ret);
    if ($row) {
      $my_count = $row['count'];
      if ($my_count != 0) {
        $errors .= "The username is not available. Please choose another username. <br>";
      }
    }

   /* 4. Username must be alphanumeric.  */

   if (!ctype_alnum($user_name)) {
     $errors .= "The username contains invalid characters<br>";
   }


    return $errors;
  }


  function saveAndRegister() {
    if (!$this->save()) {
      return false;
    }

    // If that was successful, register the user with Facebook
    $email_hash = email_get_public_hash($this->email);
    $accounts = array(array('email_hash' => $email_hash, 'account_id' => $this->username));
    return facebook_registerUsers($accounts);
  }

  function delete() {
    // Delete the account from the db
    $ret = queryf('DELETE FROM auth WHERE username = %s', $this->username);
    if (!$ret) {
      error_log("Could not delete account ($this->username)");
      return false;
    }

    // Unregister the account from fb
    return facebook_unregisterUsers(array(email_get_public_hash($this->email)));
  }

  /*
   * Tells whether the account has a password set.
   *
   * If the account has no password, then it cannot be disconnected, only
   * deleted outright (otherwise how will the user log in?)
   */
  function hasPassword() {
    return (bool) $this->password;
  }

  function getRuns() {
    if ($this->runs === null) {
      $ret = queryf('SELECT * FROM runs WHERE username = %s ORDER BY date DESC LIMIT %d',
                    $this->username, MAX_DISPLAY_RUNS);
      if (!$ret) {
        return null;
      }

      $runs = array();
      while ($row = mysql_fetch_assoc($ret)) {
        $run = new Run($this, $row);
        $runs[] = $run;
      }
    }

    $this->runs = $runs;
    return $this->runs;
  }
}
