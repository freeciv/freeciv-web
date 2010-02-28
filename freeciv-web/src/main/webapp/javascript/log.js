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


var LOG_FATAL = 0;
var LOG_ERROR = 1;		/* non-fatal errors */
var LOG_NORMAL = 2;
var LOG_VERBOSE = 3;		/* not shown by default */
var LOG_DEBUG = 4;		/* suppressed unless DEBUG defined;
				   may be enabled on file/line basis */



function freelog(level, message)
{

  console.log(message);

}

