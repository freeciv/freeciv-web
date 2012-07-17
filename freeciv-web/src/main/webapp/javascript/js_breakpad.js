/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/


var error_count = 0;
var MAX_ERROR_THRESHOLD = 1000; 
var MAX_ERROR_REPORTS = 10;
var ERROR_REPORTING_ENABLED = false;

/**************************************************************************
  Setup handling of exceptions
**************************************************************************/
function setup_crash_reporting()
{ 
  window.onerror = function(msg, url, linenumber) {
    js_breakpad_report(msg, url, linenumber);
    return true;
  };  
}

/**************************************************************************
  Report crash to server.
**************************************************************************/
function js_breakpad_report(msg, url, linenumber) 
{
  error_count += 1;
  
  if (error_count > MAX_ERROR_REPORTS) return;
  
  console.log("js_breakpad reports error: " + msg + " " + linenumber);

  if (ERROR_REPORTING_ENABLED) {
    var btr = printStackTrace().join('\n\n');
    
    $.ajax({
      url: "/js-breakpad/report.jsp",
      type: "POST",
      data: {'msg' : msg, 'url' : url, 'linenumber' : linenumber, 
             'stacktrace' : btr}      
    });
  }
    
  return true;
}

/**************************************************************************
  Is the number of errors over the threshold?
**************************************************************************/
function over_error_threshold()
{
  return (error_count > MAX_ERROR_THRESHOLD);
}
