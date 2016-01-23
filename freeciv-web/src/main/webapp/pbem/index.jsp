<!DOCTYPE html>
<html>
<head>
<title>Freeciv-web</title>
<script type="text/javascript" src="/javascript/libs/jquery.min.js"></script>
</head>
<body>
please wait...
<script>
var getUrlParameter = function getUrlParameter(sParam) {
    var sPageURL = decodeURIComponent(window.location.search.substring(1)),
        sURLVariables = sPageURL.split('&'),
        sParameterName,
        i;

    for (i = 0; i < sURLVariables.length; i++) {
        sParameterName = sURLVariables[i].split('=');

        if (sParameterName[0] === sParam) {
            return sParameterName[1] === undefined ? true : sParameterName[1];
        }
    }
};

var u = getUrlParameter('u');
if (u != null) {
  window.location="/webclient/?action=pbem&invited_by=" + u;
} else {
  alert("missing user url paramter.");
}
</script>
</body>
</html>



