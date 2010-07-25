<%@ page session="true" %>

<link rel="stylesheet" href="/stylesheets/openid.css" />

<script type="text/javascript" src="/auth/openid-jquery.js"></script>
<script type="text/javascript">
$(document).ready(function() {
    openid.init('openid_identifier');
    openid.setDemoMode(false); 
});
</script>

<%
    if (request.getParameter("logout")!=null)
    {
        session.removeAttribute("openid");
        session.removeAttribute("openid-claimed");
%>
    Logged out!<p>
<%
    }
	if (session.getAttribute("openid")==null) {
%>
<div id="main_column">

<h2>Login to play Freeciv.net</h2>
<p>Freeciv.net uses <a href="http://openid.net/">OpenID</a> to allow users to sign-in to the site.
This means that you can easily and safely use your existing username and password from Google, Yahoo, or any other valid OpenID provider to log-in.
</p>
<br/>

<!-- Simple OpenID Selector -->
<form action="/auth/consumer_redirect.jsp" method="get" id="openid_form">

	<fieldset>
    		<legend>Sign-in or Create New Account using OpenID:</legend>
    		
    		<div id="openid_choice">
	    		<p>Please click your account provider:</p>
	    		<div id="openid_btns"></div>
			</div>
			
			<div id="openid_input_area">
				<input id="openid_identifier" name="openid_identifier" type="text" value="http://" />
				<input id="openid_submit" type="submit" value="Sign-In"/>
			</div>
			<noscript>
			<p>OpenID is service that allows you to log-on to many different websites using a single indentity.
			Find out <a href="http://openid.net/what/">more about OpenID</a> and <a href="http://openid.net/get/">how to get an OpenID enabled account</a>.</p>
			</noscript>
	</fieldset>
</form>




<br><br><br>
Freeciv.net previously supported <a href="http://www.freeciv.net/facebook">Facebook connect</a>. Existing users can still log-in with the old method.

<br><br><br>

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


<%	
} else {

%>
<%--Logged in as <%= session.getAttribute("openid") %><p>
<a href="?logout=true">Log out</a>--%>

<%
  response.sendRedirect("/preload.jsp");  
%>


<% } %>
</div>
