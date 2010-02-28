<?php

/*
 * Renders the JS necessary for any Facebook interaction to work.
 */
function render_fbconnect_init_js() {
  $html = sprintf(
    '<script src="%s/js/api_lib/v0.4/FeatureLoader.js.php" type="text/javascript"></script>
     <script type="text/javascript">
       FB.init("%s", "xd_receiver.php");
     </script>
     <script src="fbconnect.js" type="text/javascript"></script>',
    get_static_root(),
    get_api_key());

  $already_logged_in = facebook_client()->get_loggedin_user() ? "true" : "false";
  onloadRegister(sprintf("facebook_onload(%s);", $already_logged_in));

  return $html;
}

/*
 * Render a custom button to log in via Facebook.
 * When the button is clicked, the Facebook JS library pops up a Connect dialog
 * to authenticate the user.
 * If the user authenticates the application, the handler specified by the
 * onlogin attribute will be triggered.
 *
 * @param $size   size of the button. one of ('small', 'medium', 'large')
 *
 */
function render_fbconnect_button($size='medium') {

return '<a href="#" onclick="FB.Connect.requireSession(); return false;" >
  <img id="fb_login_image" src="http://static.ak.fbcdn.net/images/fbconnect/login-buttons/connect_light_medium_long.gif" alt="Connect"/>
</a>';

  return '<fb:login-button '.
           'size="'.$size.'" background="light" length="long" '.
           'onlogin="facebook_onlogin_ready();"></fb:login-button>';
}

/*
 * Display the feed form when the page is loaded. This should be called only if the user has already
 * been warned that the feed form is coming (via a checkbox, button, etc), or else it can be a pretty
 * jarring experience.
 *
 * @param $run    the run to publish in feed
 */
function register_feed_form_js($run) {
  onloadRegister(sprintf("facebook_publish_feed_story(%d, %s); ",
                         get_feed_bundle_id(),
                         json_encode($run)));
}

/*
 * Get the facebook client object for easy access.
 */
function facebook_client() {
  static $facebook = null;
  if ($facebook === null) {
    $facebook = new Facebook(get_api_key(), get_api_secret());

    if (!$facebook) {
      error_log('Could not create facebook client.');
    }

  }
  return $facebook;
}

/**
 * Register new accounts with Facebook to facilitate friend linking.
 * Note: this is an optional step, and only makes sense if you have
 * a site with an existing userbase that you want to tie into
 * Facebook Connect.
 *
 * See http://wiki.developers.facebook.com/index.php/Friend_Linking
 * for more details.
 *
 * @param $accounts  array of accounts, each with keys 'email_hash' and 'account_id'
 * @return whether the emails were registered. true unless there's an error
 */
function facebook_registerUsers($accounts) {
  $facebook = facebook_client();
  $session_key = $facebook->api_client->session_key;
  $facebook->api_client->session_key = null;

  $result = false;
  try {
    $ret = $facebook->api_client->call_method(
             'facebook.connect.registerUsers',
             array('accounts' => json_encode($accounts)));

    // On success, return the set of email hashes registered
    // An email hash will be registered even if the email does not match a Facebook account

    $result = (count($ret) == count($accounts));
  } catch (Exception $e) {
    error_log("Exception thrown while calling facebook.connect.registerUsers: ".$e->getMessage());
  }

  $facebook->api_client->session_key = $session_key;
  return $result;
}

/**
 * Lets Facebook know that these emails are no longer members of your site.
 *
 * @param email_hashes   an array of strings from registerUsers
 */
function facebook_unregisterUsers($email_hashes) {
  $facebook = facebook_client();
  $session_key = $facebook->api_client->session_key;
  $facebook->api_client->session_key = null;

  // Unregister the account from fb
  $result = false;
  try {
    $ret = $facebook->api_client->call_method(
             'facebook.connect.unregisterUsers',
             array('email_hashes' => json_encode($email_hashes)));
    $result = (count($email_hashes) == count($ret));
  } catch (Exception $e) {
    error_log("Exception thrown while calling facebook.connect.unregisterUsers: ".$e->getMessage());
  }

  $facebook->api_client->session_key = $session_key;
  return $result;
}

/*
 * Fetch fields about a user from Facebook.
 *
 * If performance is an issue, then you may want to implement caching on top of this
 * function. The cache would have to be cleared every 24 hours.
 */
function facebook_get_fields($fb_uid, $fields) {
  try {
    $infos = facebook_client()->api_client->users_getInfo($fb_uid,
                                                          $fields);

    if (empty($infos)) {
      return null;
    }
    return reset($infos);

  } catch (Exception $e) {
    error_log("Failure in the api when requesting " . join(",", $fields)
              ." on uid " . $fb_uid . " : ". $e->getMessage());
    return null;
  }
}

/**
 * Returns the "public" hash of the email address, i.e., the one we give out
 * to select partners via our API.
 *
 * @param  string $email An email address to hash
 * @return string        A public hash of the form crc32($email)_md5($email)
 */
function email_get_public_hash($email) {
  if ($email != null) {
    $email = trim(strtolower($email));
    return sprintf("%u", crc32($email)) . '_' . md5($email);
  } else {
    return '';
  }
}

/*
 * Check if the current server matches with the URL configured. If not, then
 * redirect to one that does - but first put up a page with a warning about it.
 *
 * This is mostly for the sake of the demo app, to make sure you've got it setup.
 * Probably not necessary in production as long as you have your own way of making
 * sure you're on the right domain.
 */
function ensure_loaded_on_correct_url() {
  $current_url = get_current_url();
  $callback_url = get_callback_url();

  if (!$callback_url) {
    $error = 'You need to specify $callback_url in lib/config.php';
  }
  if (!$current_url) {
    error_log("therunaround: Unable to figure out what server the "
             ."user is currently on, skipping check ...");
    return;
  }

  if (get_domain($callback_url) != get_domain($current_url)) {
    // do a redirect
    $url = 'http://' . get_domain($callback_url) . $_SERVER['REQUEST_URI'];
    $error = 'You need to access your website on the same url as your callback. '
      .'Accessed at ' . get_domain($current_url) . ' instead of ' . get_domain($callback_url)
      .'. Redirecting to <a href="'.$url.'">'.$url.'</a>...';
    $redirect = '<META HTTP-EQUIV=Refresh CONTENT="10; URL='.$url.'">';
  }

  if (isset($error)) {
    echo '<head>'
      .'<title>The Run Around</title>'
      .'<link type="text/css" rel="stylesheet" href="style.css" />'
      . isset($redirect) ? $redirect : ''
      . '</head>';

    echo render_error($error);
    exit;
  }
}
