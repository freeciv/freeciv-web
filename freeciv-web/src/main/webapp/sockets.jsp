<%@ page session="false" %>
<%@page import="java.io.*"%>
<%@page import="java.util.*"%>
<%@page import="java.sql.*"%>
<%@page import="javax.servlet.*"%>
<%@page import="javax.sql.*"%>
<%@page import="javax.naming.*"%>
<%@page import="java.security.*"%>
<%@page import="java.math.*"%>
<%@page import="sun.misc.BASE64Encoder"%>
<%@page import="java.util.regex.*"%>


<script src="http://www.facebook.com/js/api_lib/v0.4/FeatureLoader.js.php" type="text/javascript"></script>

<%
  response.addHeader("Cache-Control", "max-age=60");
%>

<!-- <%= new java.util.Date() %>  -->

<br>
<!--<div id="ad_top"></div>-->


<!-- MAIN COLUMN WITH ARTICLES -->
<div id="left-column">

<h2>Benefits of HTML5 WebSockets for Freeciv.net</h2> 
<b>
The Freeciv.net project would benefit greatly from HTML5 WebSockets support in web browsers. This new web technology 
would instantly improve the user-experience, by reducing the time it takes to transmit messages between
the game server and end-users. This would result in a faster and more responsive playing experience.
Support for HTML5 WebSockets and canvas in all web browsers is probably most important features that
will make it possible to make browser-based games in HTML5 and Javascript which are as good as similar
Flash games.
 </b>

<br><br>

<b>What are WebSockets?</b><br>
"WebSockets is a technology providing for bi-directional, full-duplex communications channels, over a single Transmission Control Protocol (TCP) socket, designed to be implemented in web browsers and web servers. The WebSockets API is being standardized by the W3C and the WebSocket protocol is being standardized by the IETF", according to <a href="http://en.wikipedia.org/wiki/Web_Sockets">Wikipedia</a>.
The specification of the WebSockets API can be found <a href="http://dev.w3.org/html5/websockets/">here</a>.

<br><br>

<b>Support in current Web Browsers</b><br>
Currently, only <a href="http://wiki.whatwg.org/wiki/Implementations_in_Web_browsers#Web_Sockets">Google Chrome supports</a> WebSockets. The other major web browsers, including 
Internet Explorer, Firefox and Safari don't support HTML5 WebSockets.

<br><br>


<b>Why still no WebSockets support in Firefox? </b><br>
Christopher Blizzard has written <a href="http://hacks.mozilla.org/2010/04/websockets-in-firefox/">an article on hacks.mozilla.org</a> which summarizes why there still isn't any
support for WebSockets in Firefox: "<i>we want to ship it because the promise of WebSockets is great, but we'll have to see if it's stable and safe enough to do so. 
Code isn't the issue - we've had that for a while. It's whether or not there
is consensus on if the protocol is "done enough" to ship it to a few hundred million people.</i>"

<br><br>

<b>Current work-around: AJAX with frequent polling</b><br>
Freeciv.net currently uses the <a href="http://en.wikipedia.org/wiki/Ajax_%28programming%29">AJAX</a> techniques used on the client-side to communicate with the server.
This means that several times each second, the web browser sends a HTTP request to the server. Then it typically takes 200 milliseconds to send a request to the server,
and another 200 milliseconds for the server to respond to this request. For the client, it takes approximately 400 milliseconds between each update from the server.      
For those interested in the current system architecture of Freeciv.net, it can be <a href="http://code.google.com/p/freeciv-forever/wiki/FreecivNetSoftwareArchitecture">
read here</a>.

<br><br>

<b>Benefits of WebSockets for Freeciv.net</b><br>
The most important benefit for Freeciv.net of using HTML5 WebSockets is that it will improve
network performance and reduce network lag. The following figure compares the current method (AJAX on the HTTP protocol),
with the upcoming HTML5 WebSockets protocol. Note that the sequence diagram is simplified for understanding.
<br><br>

<img src="/images/sockets.png">
<br><br>

In this figure it is shown how the server has to transmit a message to the client using the HTTP protocol: 
the client has to send a request message to the server, and the server has to send a response to the server.
The problem here is that the server can never send a message directly to the client.
This problem is solved using WebSockets, because then the server can send messages to the client directly.
<br><br>
When an game event occurs on the server, using WebSockets, it is possible to send a message to the client immediately.
As a result, the end-users will get faster updates of server-side events, and it will use less network bandwidth.

<br><br>
<b>The Freeciv.net project needs HTML5 WebSockets now! </b><br>
To sum up, here are some wishes from the Freeciv.net project about WebSockets:
<br>
<ul>
<li>HTML5 WebSockets will make the game faster and more responsive, because the server can then send packets to the clients directly.
<li>HTML5 WebSockets must be supported as soon as possible on all new web browsers, including Interet Explorer and Firefox.
<li>HTML5 WebSockets must support gzip compression of packets, and be compatible with HTTP proxy servers.
<li>If <a href="http://www.apple.com/hotnews/thoughts-on-flash/">HTML 5 should replace Flash</a>, then good HTML5 WebSockets and Canvas support on all platforms is important.
</ul> 

<br><br>
<b>Screenshot of the Freeciv.net web client:</b><br>
<a href="/"><img src="http://freeciv.net/images/freeciv-screenshot-2.png" border="0"></a>
<br><br><br>


<b>Your feedback on HTML5 WebSockets is wanted here.</b><br>



<br><br><br>


<div id="disqus_thread"></div>
<script type="text/javascript">
  var disqus_identifier = "sock"; //[Optional but recommended: Define a unique identifier (e.g. post id or slug) for this thread] 
  (function() {
   var dsq = document.createElement('script'); dsq.type = 'text/javascript'; dsq.async = true;
   dsq.src = 'http://freecivnet.disqus.com/embed.js';
   (document.getElementsByTagName('head')[0] || document.getElementsByTagName('body')[0]).appendChild(dsq);
  })();
</script>
<noscript>Please enable JavaScript to view the <a href="http://disqus.com/?ref_noscript=freecivnet">comments powered by Disqus.</a></noscript>
<a href="http://disqus.com" class="dsq-brlink">blog comments powered by <span class="logo-disqus">Disqus</span></a>







</div>


<div id="right-column">
  <!-- RIGHT COLUMN -->
    <div id="right-top-column">

		<div id="search_box">
		  <form action="http://freeciv.net/wireframe.jsp?do=search" id="cse-search-box">
			<div>
			    <input type="hidden" name="cx" value="partner-pub-4977952202639520:pl5d86-k1oq" />
			    <input type="hidden" name="cof" value="FORID:10" />
			    <input type="hidden" name="ie" value="UTF-8" />
			    <input type="hidden" name="do" value="search" />
			    <input type="text" name="q" size="25" />
			    <input type="submit" name="sa" value="Search" />
			  </div>
			</form>
			<script type="text/javascript" src="http://www.google.com/cse/brand?form=cse-search-box&amp;lang=en"></script> 
		</div>
    </div>
    
    <div id="right-left-column">
    <div class="right-col-box">
      <div class="login_prompt"><b>Login</b> with Facebook:</div>
      <br>
	  <a href="http://www.freeciv.net/wireframe.jsp?do=login">
        <img alt="Connect" src="http://static.ak.fbcdn.net/images/fbconnect/login-buttons/connect_light_medium_long.gif" id="fb_login_image"/>
      </a>

      <br/><br/>
      <div class="login_prompt">Or login with your <b>Username</b>:</div>


      <form name="enter" action="/facebook/login.php" method="post">
      <input type="text" name="username" size="18" value="" class="login_text_input"/>
	  <label>
		Password:
	  </label>
	  <input type="password" name="password" size="18" class="login_text_input"/>
	  <br/>
      <a class="signup_link" href="/wireframe.jsp?do=login">Signup free now!</a><br/>
      <br/>
	  <input type="submit" class="inputsubmit" name="userlogin" value="Log in"/>
        
      </form>

    </div>

    <div class="right-col-box">
	<span class="right-box">Freeciv.net</span><br>
	<span class="right-box-smaller">Freeciv.net is an Open Source strategy game which can be played online against other players, 
         or in single player mode against AI opponents.</span>
    </div>




   <div class="right-col-box">
	<span class="right-box">System requirements</span>
	<span class="right-box-smaller">Freeciv.net requires a powerful system:<br>
        - Firefox, Safari, Google Chrome and Opera with Javascript enabled.<br>
        - A fast CPU and broadband connection.<br>
        - Internet Explorer 7 or 8 with <a href="code.google.com/chrome/chromeframe/">Google Chrome Frame</a> is currently experimental. <br>
	<br>

        </span>
    </div>


  </div>


  <div id="right-right-column">
  <!-- OUTER RIGHT COLUMN WITH ADS -->

    <div id="ad_right">
    
    <script type="text/javascript"><!--
	google_ad_client = "pub-4977952202639520";
	/* 120x600, opprettet 19.12.09 for freeciv.net */
	google_ad_slot = "3834791887";
	google_ad_width = 120;
	google_ad_height = 600;
	//-->
	</script>
	<script type="text/javascript"
	src="http://pagead2.googlesyndication.com/pagead/show_ads.js">
	</script>
    
    </div>
  </div>

</div>

<div style="width: 320px; float:right; padding-right: 36px;">

<div style="height: 280px;">

<script type="text/javascript">
document.write('<fb:fan profile_id="233603224506" stream="0" connections="10" width="320" logobar="1"></fb:fan>');
FB.init("c118efb4079841cf9c60eb52ece12ab2");
</script>

</div>
<div>
<script src="http://widgets.twimg.com/j/2/widget.js"></script>
<script>
new TWTR.Widget({
  version: 2,
  type: 'search',
  search: 'freeciv.net OR freecivnet OR Freeciv',
  interval: 6000,
  title: '',
  subject: 'Freeciv.net on Twitter',
  width: 320,
  height: 250,
  theme: {
    shell: {
      background: '#696969',
      color: '#eeeeee'
    },
    tweets: {
      background: '#eeeeee',
      color: '#444444',
      links: '#1985b5'
    }
  },
  features: {
    scrollbar: false,
    loop: true,
    live: true,
    hashtags: true,
    timestamp: true,
    avatars: true,
    behavior: 'default'
  }
}).render().start();
</script>
</div>

<div style="background-color:#696969; margin-top: 30px; ">


	<script type="text/javascript"><!--
	google_ad_client = "pub-4977952202639520";
	/* 300x250, opprettet 13.02.10 */
	google_ad_slot = "0432664781";
	google_ad_width = 300;
	google_ad_height = 250;
	//-->
	</script>
	<script type="text/javascript"
	src="http://pagead2.googlesyndication.com/pagead/show_ads.js">
	</script>



</div>


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


<script type="text/javascript">
//<![CDATA[
(function() {
	var links = document.getElementsByTagName('a');
	var query = '?';
	for(var i = 0; i < links.length; i++) {
	if(links[i].href.indexOf('#disqus_thread') >= 0) {
		query += 'url' + i + '=' + encodeURIComponent(links[i].href) + '&';
	}
	}
	document.write('<script charset="utf-8" type="text/javascript" src="http://disqus.com/forums/freecivnet/get_num_replies.js' + query + '"></' + 'script>');
})();
//]]>
</script>

