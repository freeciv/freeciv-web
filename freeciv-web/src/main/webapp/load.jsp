
<div id="main_column">

<div style="text-align: center;">
<center>


<%	
  String guest_mode = (String)request.getSession().getAttribute("guest_user");	 	
  if ((guest_mode == null || guest_mode == "true")) { 
%>
<br>
<b>To load games, please login using OpenID, then click the "Load Game" button again.</b>
<br>
<jsp:include page="/auth/index.jsp" flush="false"/>

<%
  } else {
%>

<h2>Load Game</h2>

  <jsp:include page="/list_savegames.jsp" flush="false"/>
  
  <br><br><br>Note that your savegames are stored for 30 days, then they will be deleted.<br>


<script>
	$( ".button").button();
	$( ".button").css("width", "400px");
	$( ".button").css("margin", "10px");
</script>

<% } %>
</div>
</div>

