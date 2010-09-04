var mapview_canvas = null;
var ctx = null;
var canvas_text_font = "32pt Arial"; // with canvas text support
var canvas_text_small = "16pt Arial"; // with canvas text support
var globe_x = 280;
var globe_y = 100;
var logo_x = 300;
var logo_y = 360;
var text_x = 400;
var text_y = 420;

var img = null;

var intro_text = [" ", 
                  "Welcome to Freeciv.net!", 
                  "Freeciv.net is a strategy game where your goal is to conquer the world.", 
		  "Please wait while the game is loading...", 
		  "Press F11 now to play the game in fullscreen mode.",
		  "Visit www.freeciv.net to play and learn more about the game."
		  ];
var xtext = 0;

function supports_canvas() {
  return !!document.createElement('canvas').getContext;
}

function is_iphone()
{
  var agent=navigator.userAgent.toLowerCase();
  return (agent.indexOf('iphone')!=-1);
  //return true;
}

function fc_redirect() 
{
  if (is_iphone()) {
    window.location='/civclientlauncher?action=new'
  } else {
    window.location='/wireframe.jsp?do=menu'
  }
}

function run_intro() 
{
  if (!supports_canvas()) return;
      
  mapview_canvas = document.getElementById('canvas');
  ctx = mapview_canvas.getContext("2d");

  var has_canvas_text_support = (ctx.fillText && ctx.measureText);
  if (!has_canvas_text_support) return;

  img = new Image();  
  img.onload = function(){  
    setInterval ("intro_loop();", 500);
    setInterval ("inc_text();", 4000);


  }  
  img.src = '/images/freeciv-splash.jpg';   

}

function inc_text() 
{
  xtext = (xtext + 1)  % intro_text.length;
}


function intro_loop() 
{

  globe_x = globe_x + Math.floor(Math.random() + 0.05);
  globe_y = globe_y + Math.floor(Math.random() + 0.1);

  ctx.fillStyle = "rgba(0, 0, 0, 1)";
  ctx.fillRect (0, 0, 800, 600);

  ctx.drawImage(img, globe_x, globe_y);  
  put_text_header("Freeciv.net", logo_x, logo_y);

  put_text_small(intro_text[xtext], text_x, text_y);


}


/**************************************************************************
  Draw city text onto the canvas.
**************************************************************************/
function put_text_header(text, canvas_x, canvas_y) {

    ctx.font = canvas_text_font;
    ctx.fillStyle = "rgba(0, 0, 0, 1)";
    ctx.fillText(text, canvas_x-2, canvas_y-2);  
    ctx.fillStyle = "rgba(255, 255, 255, 1)";
    ctx.fillText(text, canvas_x, canvas_y);  
}

/**************************************************************************
  Draw city text onto the canvas.
**************************************************************************/
function put_text_small(text, canvas_x, canvas_y) {

    ctx.font = canvas_text_small;
    var width = ctx.measureText(text).width;

    ctx.fillStyle = "rgba(0, 0, 0, 1)";
    ctx.fillText(text, canvas_x - 2 - Math.floor(width / 2) - 5, canvas_y-2);  
    ctx.fillStyle = "rgba(255, 255, 255, 1)";
    ctx.fillText(text, canvas_x - Math.floor(width / 2) - 5, canvas_y);  
}

run_intro();
