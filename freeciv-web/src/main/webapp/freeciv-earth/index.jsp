<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>
<%@ include file="../WEB-INF/jsp/fragments/i18n.jsp" %>
<!DOCTYPE html>
<html lang="en">
  <head>
    <c:set var="title" scope="request" value="Freeciv-Web: Real-Earth Map - design your own world map!"/>
    <%@ include file="../WEB-INF/jsp/fragments/head.jsp" %>

<script src='https://api.mapbox.com/mapbox.js/v2.2.3/mapbox.js'></script>
<link href='https://api.mapbox.com/mapbox.js/v2.2.3/mapbox.css' rel='stylesheet' />
<script src='https://api.mapbox.com/mapbox.js/plugins/leaflet-image/v0.0.4/leaflet-image.js'></script>
<style>
  body { margin:0; padding:0; }

  .btn-primary {
   display: block;
   font-weight: 700;
   text-transform: uppercase;
   background: #be602d;
   text-shadow:
     -0.5px -0.5px 0 #000,
      0.5px -0.5px 0 #000,
     -0.5px  0.5px 0 #000,
      0.5px  0.5px 0 #000;
  }

  #map { 
   width:1000px; height: 600px; 
  }

@media (max-width: 1000px) {
  #map { 
   width:100%; height: 350px; 
  }
}
</style>

  </head>
  <body>
  <script type="text/javascript" src="//s7.addthis.com/js/300/addthis_widget.js#pubid=ra-553240ed5ba009c1" async="async"></script>

<%@ include file="../WEB-INF/jsp/fragments/header.jsp" %>

    <div class="row">
    <br><br><br><br>
    </div>

    <!-- Begin page content -->
    <div id="content" class="container">

    <div class="row">
        <div class="col-lg-12">
		<div id='map'></div>
        </div>
    </div>

    <div class="row">
        <div class="col-lg-8">
		<h2>Play Freeciv-web on the real world map!</h2>
		<b>You can now play Freeciv-web anywhere in the world map! Simply use the map above to find the location on the world map that you want to play on, then click the "Play this map" button to play Freeciv-web on the map you chose. You can for example use this map to recreate historic nations with realistic maps, or build your own nation right on your neighbourhoods map. Generating the map can take some time because it is a lot of work for your computer.</b>
        </div>
        <div class="col-lg-4">
		<br>
                <a id="geolocate" class="btn btn-info">Find your map position</a>
        <br><br>
		<a id="snap" class="btn btn-primary btn-lg"><i class="fa fa-flag"></i> Play this map (2D Isometric)</a>
		<br>
		<a id="snap3d" class="btn btn-primary btn-lg"><i class="fa fa-cube"></i> Play this map (3D WebGL)</a>
		<div id="snap_status"></div>

        <br><br>
        <b>Map width:</b> <input id="xsize" type="number" name="width" maxlength="3" min="20" max="200" value="84"> tiles.
        <br><br>
        <b>Map height:</b> <input id="ysize" type="number" name="height" maxlength="3" min="20" max="200" value="56"> tiles.
        <br><br>
            Max 180 x 100.   (18 000 tiles).


        </div>
    </div>

    <div class="row">
        <div class="col-lg-12">
<br><br><br>
<br><br><br>
<script async src="//pagead2.googlesyndication.com/pagead/js/adsbygoogle.js"></script>
<ins class="adsbygoogle"
     style="display:block"
     data-ad-client="ca-pub-5520523052926214"
     data-ad-slot="7043279885"
     data-ad-format="auto"></ins>
<script>
(adsbygoogle = window.adsbygoogle || []).push({});
</script>
        </div>
    </div>


<%@ include file="../WEB-INF/jsp/fragments/footer.jsp" %>

    </div>



    <script type="text/javascript" src="/javascript/libs/json3.min.js"></script>
    <script type="text/javascript" src="/javascript/libs/simpleStorage.min.js"></script>
    <script type="text/javascript" src="/freeciv-earth/earth.js"></script>

  </body>
</html>

