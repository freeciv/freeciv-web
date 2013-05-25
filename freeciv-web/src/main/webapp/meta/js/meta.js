/* javascript for metaserver page. */
$(document).ready(function() {

	$( ".button").button();
	$( ".button").css("font-size", "13px");
	$(".button").css("margin", "7px");
	if (window.location.href.indexOf("tab") != -1) {
		$( "#tabs" ).tabs({ active: 1 });
	 } else { 
		$( "#tabs" ).tabs();
	}

        if ($(window).width() <= 1024) {
          // for small screens
          $(".row").width("100%");
          $(".span10").width("100%");
          $(".span10").css("padding", "0px");
          $("#singleplr").html("Singleplayer");
          $("#multiplr").html("Multi");
          $("#freecivmeta").html("Desktop");
        }
});

