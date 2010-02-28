function populate_date(month, day, year) {
  ge('date_month').value = month;
  ge('date_day').value = day;
  ge('date_year').value = year;
}

function ge(elem) {
  return document.getElementById(elem);
}

/*
 * Simple Ajax call method.
 *
 * From http://en.wikipedia.org/wiki/XMLHttpRequest
 */
function ajax(url, vars, callbackFunction) {
  var request =  new XMLHttpRequest();
  request.open("POST", url, true);
  request.setRequestHeader("Content-Type",
                           "application/x-www-form-urlencoded");

  request.onreadystatechange = function() {
    if (request.readyState == 4 && request.status == 200) {
      if (request.responseText) {
        callbackFunction(request.responseText);
      }
    }
  };
  request.send(vars);
}
