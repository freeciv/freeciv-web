<?php

/*
 * Show the header that goes at the top of each page.
 */
function render_header() {
  // Might want to serve this out of a canvas sometimes to test
  // out fbml, so if so then don't serve the JS stuff
  if (isset($_POST['fb_sig_in_canvas'])) {
    return;
  }

  prevent_cache_headers();

  $html = '

<!DOCTYPE html>
<html>
<head>
<title>Freeciv.net - online multiplayer strategy game</title>

<link type="text/css" rel="stylesheet" href="style.css" />
<script type="text/javascript" src="base.js"></script>

<link href="/stylesheets/frontpage.css" rel="stylesheet" type="text/css" />';

if ($_COOKIE["facebook_mode"] == "true") {
  $html .= '<link href="/stylesheets/fb_frontpage.css" rel="stylesheet" type="text/css" />';
}

$html .= '<link rel="shortcut icon" href="/images/freeciv-forever-icon.png" />

<script type="text/javascript" src="/javascript/jquery-1.4.1.min.js"></script>

</head>
<body>
<!-- HEADER -->
<div id="container">

<div id="header">

<div id="header_menu"> <a class="menu_link" href="/?fb=true" title="News about Freeciv.net">News</a> &nbsp;&nbsp; 
<a class="menu_link" href="/wireframe.jsp?do=login" title="Login to play Freeciv.net now">Play Now</a> &nbsp;&nbsp; 
<a class="menu_link" href="/freecivmetaserve/metaserver.php" title="Multiplayer Games">Games</a> &nbsp;&nbsp; 
<a class="menu_link" href="/wiki" title="Documentation">Documentation</a> &nbsp;&nbsp; 
<a class="menu_link" href="/forum/" title="Freeciv.net Forum">Forum</a> &nbsp;&nbsp;
<a class="menu_link" title="Contribute to the Freeciv.net development" href="/wireframe.jsp?do=dev">Development</a> &nbsp;&nbsp;
<a title="About Freeciv.net" class="menu_link" href="/wireframe.jsp?do=about">About</a></div>
</div>

<div id="body_content">
<br>

<div id="main_column">
  ';

  $html .= '<div class="body_content">';


  // Catch misconfiguration errors.
  if (!is_config_setup()) {
    $html .= render_error('Your configuration is not complete. '
                          .'Follow the directions in <tt><b>lib/config.php.sample</b></tt> to get set up');
    $html .= '</body>';
    echo $html;
    exit;
  }

  return $html;
}

/*
 * Prevent caching of pages. When the Javascript needs to refresh a page,
 * it wants to actually refresh it, so we want to prevent the browser from
 * caching them.
 */
function prevent_cache_headers() {
  header('Cache-Control: private, no-store, no-cache, must-revalidate, post-check=0, pre-check=0');
  header('Pragma: no-cache');
}

/*
 * Register a bit of javascript to be executed on page load.
 *
 * This is printed in render_footer(), so make sure to include that on all pages.
 */
function onloadRegister($js) {
  global $onload_js;
  $onload_js .= $js;
}

/*
 * Print the unified footer for all pages. Includes all onloadRegister'ed Javascript for the page.
 *
 */
function render_footer() {
  global $onload_js;
  $html = '</div>'; 

  // the init js needs to be at the bottom of the document, within the </body> tag
  // this is so that any xfbml elements are already rendered by the time the xfbml
  // rendering takes over. otherwise, it might miss some elements in the doc.
  if (is_fbconnect_enabled()) {
    $html .= render_fbconnect_init_js();
  }

  // Print out all onload function calls
  if ($onload_js) {
    $html .= '<script type="text/javascript">'
      .'window.onload = function() { ' . $onload_js . ' };'
      .'</script>';
  }

  $html .= '




</div>

	<div id="ad_bottom">
		<script type="text/javascript"><!--
		google_ad_client = "pub-4977952202639520";
		/* 728x90, opprettet 19.12.09 for freeciv.net */
		google_ad_slot = "9174006131";
		google_ad_width = 728;
		google_ad_height = 90;
		//-->
		</script>
		<script type="text/javascript"
		src="http://pagead2.googlesyndication.com/pagead/show_ads.js">
		</script>
	</div>

</body></html>';

  return $html;
}

/*
 * Default index for logged out users.
 *
 */
function render_logged_out_index() {

  $html = '<div class="welcome_dialog">';
  $html .= '<h3>Welcome to Freeciv.net </h3>';

  $html .= '<p>Please login here, using either a username and password you 
            have created, or click the "Connect with Facebook" button to login using your Facebook account.</p>';


  $html .= '<div class="clearfix"><form action="login.php" method="post">'
    . '<div class="login_sector">'
    . '<div class="login_prompt"><b>Login</b>:</div>'
    .'<div class="clearfix"><label>Username:</label><input name="username" class="inputtext" type="text" size="20" value="" /></div> '
      .'<div class="clearfix"><label>Password:</label><input name="password" class="inputtext" type="password" size="20" value=""/></div> '
      .'<input id="submit" class="inputsubmit" value="Login" name="submit" type="submit" />'
    . '</div>';

  if (is_fbconnect_enabled()) {
    $html .= '<div class="login_sector_fb">';
    $html .= '<div class="login_prompt">Or <b>login</b> with Facebook:</div>';
    $html .= render_fbconnect_button('medium');
    $html .= '</div>';
  }

  $html .= '</form></div>';


  $html .= '<div class="signup_container"> '
    .'Don\'t have an account? <a href="register.php">Register Now!</a> ';

  $html .= '</div></div>';

  return $html;
}

function render_add_run_table($user) {
  $html  = '<h3>Where did you run recently?</h3>';
  $html .= '<form action="index.php" method="post">';
  $html .= render_add_run_table_fields();
  if ($user->is_facebook_user()) {
    $style = '';
  } else {
    $style = 'visibility:hidden';
    onloadRegister('facebook_show_feed_checkbox();');
  }
  $html .= '<p id="publish_fb_checkbox" style="'.$style.'" >'
      .'<img src="http://static.ak.fbcdn.net/images/icons/favicon.gif" /> '
      .'<input type="checkbox" name="publish_to_facebook" checked /> '
      .'Publish this run to Facebook'
      .'</p>';
  $html .= render_input_button('Add Run', 'submit');
  $html .= '</form>';

  return $html;
}

/*
 * Renders input fields for adding run.  Used by both index.php and
 * handlers/self_publisher.php.
 */
function render_add_run_table_fields() {
  $html  = '<table class="add_run_table">';
  $html .= render_text_editor_row('route', 'Where did you go?');
  $html .= render_text_editor_row('miles', 'Number of Miles');
  $html .= '<tr><td class="editor_key"><label>Date (MM/DD/YYYY)</label></td>'
    .'<td class="editor_value">'
    .'<input id="date_month" class="inputtext datefield" name="date_month" type="text" size="2" maxlength="2" /> '
    .'/<input id="date_day" class="inputtext datefield" name="date_day" type="text" size="2" maxlength="2" /> '
    .'/<input id="date_year" class="inputtext datefield" name="date_year" type="text" size="4" maxlength="4" /> '
    . ' | ' . render_populate_date_link('Today')
    . ' | ' . render_populate_date_link('Yesterday')
    .'</td>'
    .'</tr>';
  $html .= '</table>';
  return $html;
}

/*
 * Form for editing the user's account info.
 */
function render_edit_user_table ($user) {
  $html .= '<form action="account.php" method="post">';
  $html .= '<input type="hidden" name="username" value="'.$user->username.'" >';
  $html .= '<table class="editor">';

  if ($user->is_facebook_user()) {
    $name = $user->getName() . ' &nbsp;<img src="http://static.ak.fbcdn.net/images/icons/favicon.gif" />';
    $email = '<b>Contact via Facebook</b>';
  } else {
    $name = '<input id="name" class="inputtext" type="text" size="20" value="'.$user->getName().'" name="name">';
    $email = '<input id="email" class="inputtext" type="text" size="20" value="'.$user->getEmail().'" name="email">';
  }

  $html .= '<tr>'
    .'<td><label id="label_name" for="name">Name</label></td>'
    .'<td>'.$name.'</td>'
    .'</tr>'
    .'<tr>'
    .'<td><label id="label_email" for="email">Email</label></td>'
    .'<td>'.$email.'</td>'
    .'</tr>'
    .'<tr>'
    .'<td><label id="label_email_settings" for="email_settings">Email Settings</label></td>'
    .'<td>'
    .'<a href="#" onclick="facebook_prompt_permission(\'email\'); return false;">Receive Email Updates</a>'
    .'</td>'
    .'</tr>';

  if ($user->hasPassword()) {
    $html .= '<tr>'
      .'<td><label id="label_password" for="password">Password</label></td>'
      .'<td><input id="password" class="inputtext" type="password" size="20" value="'.PASSWORD_PLACEHOLDER.'" name="password">'
      .'</tr>';
  }

  $html .= '</table>';
  $html .= render_input_button('Save Changes', 'submit');
  $html .= '</form>';
  return $html;
}

function render_populate_date_link($datestr) {
  $time = strtotime($datestr);
  $month = date('m', $time);
  $day = date('d', $time);
  $year = date('Y', $time);
  return '<a onclick="populate_date(\''.$month.'\', \''.$day.'\', \''.$year.'\'); return false;">'.$datestr.'</a>';
}

function render_text_editor_row($id, $label, $value='', $size=20, $after_input='') {
  return '<tr><td class="editor_key">'
    .'<label id="label_'.$id.'" for="'.$id.'">'.$label.'</label>'
    .'</td><td class="editor_value">'
    .'<input id="'.$id.'" class="inputtext" type="text" size="'.$size.'" value="'.$value.'" name="'.$id.'"/>'
    .$after_input
    .'</td></tr>';
}

function render_input_button($label, $name) {
  return '<input class="inputsubmit" type="submit" name="'.$name.'" value="'.$label.'"/>';
}

function render_error($msg) {
  return '<div class="error">'.$msg.'</div>';
}
function render_success($msg) {
  return '<div class="success">'.$msg.'</div>';
}

function render_run($run) {
  return '<tr><td>'
    .date('m/d/Y', $run->date)
    .'</td><td>'
    .$run->miles . ' miles'
    .'</td><td>'
    .$run->route
    .'</td></tr>';
}

function render_friends_table($friends) {

  if (empty($friends)) {
    return '';
  }

  $html = '';
  $html .= '<table class="friends_table">';

  foreach ($friends as $friend) {
    //$friend_as_user = User::getFacebookUser($friend);
      if (!empty($friend['pic'])) {
        $html .= "<img src='" . $friend['pic'] .  "'>" . $friend['name'] . "<br";
      } else {
        $html .= $friend['name'] . "<br";
      }
  }

  $html .= '</table>';
  return $html;
}

function render_connect_invite_link($has_existing_friends = false) {
  $more = $has_existing_friends ? ' more' : '';
  $num = '<fb:unconnected-friends-count></fb:unconnected-friends-count>';

  $one_friend_text = 'You have one'.$more.' Facebook friend that also uses The Run Around. ';
  $multiple_friends_text = 'You have '.$num.$more.' Facebook friends that also use The Run Around. ';
  $invite_link = '<a onclick="FB.Connect.inviteConnectUsers(); return false;">Invite them to Connect.</a>';

  $html = '';
  $html .= '<fb:container class="HideUntilElementReady" condition="FB.XFBML.Operator.equals(FB.XFBML.Context.singleton.get_unconnectedFriendsCount(), 1)" >';
  $html .= '<span>'.$one_friend_text.' '.$invite_link.'</span>';
  $html .= '</fb:container>';
  $html .= '<fb:container class="HideUntilElementReady" condition="FB.XFBML.Operator.greaterThan(FB.XFBML.Context.singleton.get_unconnectedFriendsCount(), 1)" >';
  $html .= '<span>'.$multiple_friends_text.' '.$invite_link.'</span>';
  $html .= '</fb:container>';
  return $html;
}
