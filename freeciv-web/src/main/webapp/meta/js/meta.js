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
});

