<%
  response.addHeader("Cache-Control", "max-age=60");
%>

<br>

<%
  String server = request.getServerName();

  if (server.equals("localhost") 
   || server.equals("www.freeciv.net") 
   || server.equals("freeciv.net")
   || server.equals("games.freeciv.net")) { %> 

  <jsp:include page="/wireframe.jsp?do=frontpage" flush="false"/>
  
<% } else if (server.equals("de.freeciv.net")) {%>
  <jsp:include page="frontpage_de.jsp" flush="false"/>

<% } else if (server.equals("zh.freeciv.net")) {%>
  <jsp:include page="frontpage_zh.jsp" flush="false"/>
  
<% } else if (server.equals("es.freeciv.net")) {%>
  <jsp:include page="frontpage_es.jsp" flush="false"/>
  
<% } else if (server.equals("ja.freeciv.net")) {%>
  <jsp:include page="frontpage_ja.jsp" flush="false"/>
  
<% } else if (server.equals("ar.freeciv.net")) {%>
  <jsp:include page="frontpage_ar.jsp" flush="false"/>
  
<% } else if (server.equals("fr.freeciv.net")) {%>
  <jsp:include page="frontpage_fr.jsp" flush="false"/>

<% } else if (server.equals("no.freeciv.net")) {%>
  <jsp:include page="frontpage_no.jsp" flush="false"/>

<% } else if (server.equals("ru.freeciv.net")) {%>
  <jsp:include page="frontpage_ru.jsp" flush="false"/>

<% } %>




<object width="548" height="436"><param name="movie" value="http://www.youtube.com/v/p-nkr_Cz1O4?fs=1&amp;hl=nb_NO"></param><param name="allowFullScreen" value="true"></param><param name="allowscriptaccess" value="always"></param><embed src="http://www.youtube.com/v/p-nkr_Cz1O4?fs=1&amp;hl=nb_NO" type="application/x-shockwave-flash" allowscriptaccess="always" allowfullscreen="true" width="548" height="436"></embed></object>

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
    



  </div>


  <div id="right-right-column">
  <!-- OUTER RIGHT COLUMN WITH ADS -->
	<div style="margin-left: -30px">

		<a href="/wireframe.jsp?do=login"><img src="/images/freeciv-screenshot-fp2.png" border="0"></a>
		<br>
		<br><br>
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

<div style="background-color:#696969; ">


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

