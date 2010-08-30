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

<%
  response.addHeader("Cache-Control", "max-age=60");
%>

<!-- <%= new java.util.Date() %>  -->

<script type="text/javascript">
  if (is_iphone()) window.location="/iphone";
</script>

<%
   Context env = (Context)(new InitialContext().lookup("java:comp/env"));
   DataSource ds = (DataSource)env.lookup("jdbc/freeciv_mysql"); 
   Connection conn = ds.getConnection();
   int playercount = 0;
   try {   
       PreparedStatement stmt = conn.prepareStatement("SELECT count(*) FROM auth");
       ResultSet rs = stmt.executeQuery();
       rs.next();
       playercount = rs.getInt(1);
   } finally {
     conn.close();
   }       
%>			

<br>
<!--<div id="ad_top"></div>-->


<!-- MAIN COLUMN WITH ARTICLES -->
<div id="left-column">
	
<a href="http://www.freeciv.net/tournament"><img src="/images/freeciv-tournament.png"></a>
	
<a class="title" href="/wireframe.jsp?do=login"><span class="title">Play in the Freeciv.net Tournament now!</span></a>
<br>
<span class="article">
	Now you can compete in the Freeciv.net tournament, where you play against all other Freeciv players online. 
	The tournament is organized as a series of duel-games, and the results of these games are ranked in a Elo rating system.
<a href="http://www.freeciv.net/tournament/">Click here</a> to participate in the Freeciv tournament.
</span>


<br><br><br>
	
	
	
<a href="/wireframe.jsp?do=login"><img src="/images/freeciv-screenshot-2.png"></a>
	
<a class="title" href="/wireframe.jsp?do=login"><span class="title">Play Freeciv.net online now!</span></a>
<br>
<span class="article">
Freeciv.net is an Open Source strategy game which can be played online against other players, or in single player mode against AI opponents.
</span>


<br><br><br>
<span class="title"><a href="/wireframe.jsp?do=login">Freeciv.net now supports OpenID for signing in</a></span>
<br>

<span class="date">July 25, 2010</span>
<br>
<span class="article">
	Freeciv.net now supports <a href="http://www.openid.net">OpenID</a> for logging in to games. This means that you can log-in using
	your username and password from Google, Yahoo, AOL or any other valid OpenID provider. The old method using Facebook Connect
	is now deprecated. Please let us know what you think about the new log-in method on Freeciv.net!
	<br>
	<img src="/images/openid_logo_big.png">

</span>




<br><br><br>
<span class="title"><a href="http://arstechnica.com/microsoft/news/2010/06/ie9-platform-preview-3-video-audio-canvas-and-fonts-too.ars">Internet Explorer 9 will support HTML5 Canvas natively</a></span>
<br>

<span class="date">June 29, 2010</span>
<br>
<span class="article">
Microsoft has announced that Internet Explorer 9 will fully support the HTML5 canvas element. Therefore, Freeciv.net will also be fully supported in the next version of Interet Explorer. This is truely good news!

</span>






<br><br><br>
<span class="title">New version of Freeciv.net released today</span>
<br>

<span class="date">February 14, 2010</span>
<br>
<span class="article">
A new version of Freeciv.net has been released today. The new version features numerous bug-fixes,
improved fog-of-war rendering and 4 new scenario maps.
</span>


<br><br><br>

<a href="http://www.facebook.com/freeciv/"><img src="/images/facebookimg2.png"></a>
<a class="title" href="http://www.facebook.com/freeciv/"><span class="title">Freeciv.net now also on Facebook</span></a>
<br>
<br>
<span class="article">
Freeciv.net can now also be found on Facebook. This allows you to 
play against your friends on Facebook, and join the community
of Freeciv.net fans.
</span>


<br><br><br>

<a href="http://twitter.com/freecivnet"><img src="/images/twitterimg2.png"></a>
<a class="title" href="http://twitter.com/freecivnet"><span class="title">Follow Freeciv.net on Twitter!</span></a>
<br>
<br>
<span class="article">
Freeciv.net is now also on Twitter. This is a microblogging service where you can
get updates about this project and related topics on Twitter. 
</span>




<br><br><br>
<br><br><br>
<br><br><br>



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
			    <input type="text" name="q" size="23" />
			    <input type="submit" name="sa" value="Search" />
			  </div>
			</form>
			<script type="text/javascript" src="http://www.google.com/cse/brand?form=cse-search-box&amp;lang=en"></script> 
		</div>
    </div>
    
    <div id="right-left-column">
    <div class="right-col-box" style="background-color: #000000;">
	<a id="login_button" href="/wireframe.jsp?do=login"></a>
    </div>

    <div class="right-col-box">
	<span class="right-box">Freeciv.net</span><br>
	<span class="right-box-smaller">Freeciv.net is an Open Source strategy game which can be played online against other players, 
         or in single player mode against AI opponents.</span>
    </div>

   <div class="right-col-box">
	<span class="right-box">Community</span>
	<br>
	<span class="right-box-smaller">
	There are now <%= "" + playercount %> registered players on Freeciv.net
	<br>

        </span>
    </div>


   <div class="right-col-box">
        <span class="right-box-smaller">
		Please support Freeciv.net
		<br>
		<br>
		<center>


<script type="text/javascript">
	var flattr_url = 'http://www.freeciv.net';
</script>
<script src="http://api.flattr.com/button/load.js" type="text/javascript"></script>




	</center>

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

<div style="height: 430px;">

<script src="http://www.facebook.com/js/api_lib/v0.4/FeatureLoader.js.php" type="text/javascript"></script>
<script type="text/javascript">
document.write('<fb:fan profile_id="233603224506" stream="0" connections="15" width="320" logobar="1" height="430"></fb:fan>');
FB.init("c118efb4079841cf9c60eb52ece12ab2");
</script>

</div>


<div>
<script src="http://widgets.twimg.com/j/2/widget.js"></script>
<script>
new TWTR.Widget({
  version: 2,
  type: 'search',
  search: 'freeciv',
  interval: 6000,
  title: '',
  subject: 'Freeciv.net on Twitter',
  width: 320,
  height: 450,
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

<div style="background-color:#696969; margin-top: 20px; ">
Link partners:
<br>

<a href="http://www.bgroom.com">Free backgammon online</a>
<br>
<br>

</div>

<div style="background-color:#696969; margin-top: 30px; padding: 3px;">
<span class="right-box">Download Freeciv desktop version</span><br>
	<span class="right-box-smaller">
		Freeciv.net can be played online in your browser on this website.
		However, if you want to play the desktop version of Freeciv on your computer, you can <br>
		<a href="http://sourceforge.net/projects/freeciv/files/Freeciv%202.1/2.1.10/Freeciv-2.1.10-win32-gtk2-setup.exe/download" target="_new">Download the desktop version</a>.

	</span>

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


