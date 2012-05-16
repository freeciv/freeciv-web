/********************************************************************** 
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef AUDIO_SDL
#include "SDL.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xatom.h>

#include "canvas.h"
#include "pixcomm.h"

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "support.h"

/* common */
#include "game.h"
#include "government.h"
#include "map.h"
#include "unitlist.h"
#include "version.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "clinet.h"
#include "control.h"
#include "editgui_g.h"
#include "ggz_g.h"
#include "options.h"
#include "text.h"
#include "tilespec.h"

/* gui-xaw */
#include "actions.h"
#include "colors.h"
#include "dialogs.h"
#include "graphics.h"
#include "gui_stuff.h"		/* I_SW() */
#include "mapview.h"
#include "menu.h"
#include "optiondlg.h"
#include "pages.h"
#include "resources.h"

#include "gui_main.h"

const char *client_string = "gui-xaw";

const char * const gui_character_encoding = NULL;
const bool gui_use_transliteration = TRUE;

static AppResources appResources;

/* ids of the units icons in information display: (or 0) */
static int unit_ids[MAX_NUM_UNITS_BELOW];  

static void setup_widgets(void);

void fill_econ_label_pixmaps(void);
void fill_unit_below_pixmaps(void);

/**************************************************************************
...
**************************************************************************/
XtResource resources[] = {
    { "GotAppDefFile", "gotAppDefFile", XtRBoolean, sizeof(Boolean),
      XtOffset(AppResources *,gotAppDefFile), XtRImmediate, (XtPointer)False},
    { "version", "Version", XtRString, sizeof(String),
      XtOffset(AppResources *,version), XtRImmediate, (XtPointer)False},
#ifdef UNUSED
    { "log", "Log", XtRString, sizeof(String),
      XtOffset(AppResources *,logfile), XtRImmediate, (XtPointer)False},
    { "name", "name", XtRString, sizeof(String),
      XtOffset(AppResources *,name), XtRImmediate, (XtPointer)False},
    { "port", "Port", XtRInt, sizeof(int),
      XtOffset(AppResources *,port), XtRImmediate, (XtPointer)False},
    { "server", "Server", XtRString, sizeof(String),
      XtOffset(AppResources *,server), XtRImmediate, (XtPointer)False},
    { "metaserver", "Metaserver", XtRString, sizeof(String),
      XtOffset(AppResources *,metaserver), XtRImmediate, (XtPointer)False},
    { "logLevel", "LogLevel", XtRString, sizeof(String),
      XtOffset(AppResources *,loglevel_str), XtRImmediate, (XtPointer)False},
    { "showHelp", "ShowHelp", XtRBoolean, sizeof(Boolean),
      XtOffset(AppResources *,showHelp), XtRImmediate, (XtPointer)False},
    { "showVersion", "ShowVersion", XtRBoolean, sizeof(Boolean),
      XtOffset(AppResources *,showVersion), XtRImmediate, (XtPointer)False},
    { "tileset", "TileSet", XtRString, sizeof(String),
      XtOffset(AppResources *,tileset), XtRImmediate, (XtPointer)False},
#endif
};

/**************************************************************************
...
**************************************************************************/
#ifdef UNUSED
static XrmOptionDescRec cmd_options[] = {
/* { "-help",    ".showHelp",    XrmoptionNoArg,  (XPointer)"True" },*/
 { "-log",     ".log",         XrmoptionSepArg, (XPointer)"True" },
 { "-name",    ".name",        XrmoptionSepArg, (XPointer)"True" },
 { "-port",    ".port",        XrmoptionSepArg, (XPointer)"True" },
 { "-server",  ".server",      XrmoptionSepArg, (XPointer)"True" },
 { "-metaserver",".metaserver",XrmoptionSepArg, (XPointer)"True" },
 { "-debug",   ".logLevel",    XrmoptionSepArg, (XPointer)"True" },
 { "-tiles",   ".tileset",     XrmoptionSepArg, (XPointer)"True" },
/* { "-version", ".showVersion", XrmoptionNoArg,  (XPointer)"True" },*/
/* { "--help",    ".showHelp",    XrmoptionNoArg,  (XPointer)"True" },*/
 { "--log",     ".log",         XrmoptionSepArg, (XPointer)"True" },
 { "--name",    ".name",        XrmoptionSepArg, (XPointer)"True" },
 { "--port",    ".port",        XrmoptionSepArg, (XPointer)"True" },
 { "--debug",   ".logLevel",    XrmoptionSepArg, (XPointer)"True" },
 { "--server",  ".server",      XrmoptionSepArg, (XPointer)"True" },
 { "--metaserver",".metaserver",XrmoptionSepArg, (XPointer)"True" },
 { "--tiles",   ".tileset",     XrmoptionSepArg, (XPointer)"True" }
/* { "--version", ".showVersion", XrmoptionNoArg,  (XPointer)"True" }*/
};
#endif

/**************************************************************************/

static void end_turn_callback(Widget w, XtPointer client_data,
			      XtPointer call_data);

static void timer_callback(XtPointer client_data, XtIntervalId *id);

/**************************************************************************/

struct game_data {
  int year;
  int population;
  int researchedpoints;
  int money;
  int tax, luxurary, research;
};

/**************************************************************************/

Display	*display;
int display_depth;
int screen_number;
enum Display_color_type display_color_type;
XtAppContext app_context;

/* this GC will be the default one all thru freeciv */
GC civ_gc; 

/* This is for drawing border lines */
GC border_line_gc; 

/* and this one is used for filling with the bg color */
GC fill_bg_gc;
GC fill_tile_gc;
Pixmap gray50,gray25;

/* overall font GC                                    */
GC font_gc;
XFontSet main_font_set;
/* productions font GC                                */
GC prod_font_gc;
XFontSet prod_font_set;

Widget toplevel, main_form, menu_form, below_menu_form, left_column_form;
Widget bottom_form;
Widget map_form;
Widget map_canvas;
Widget overview_canvas;
Widget map_vertical_scrollbar, map_horizontal_scrollbar;
Widget inputline_text, outputwindow_text;
Widget econ_label[10];
Widget turn_done_button;
Widget info_command;
Widget bulb_label, sun_label, flake_label, government_label, timeout_label;
Widget unit_info_label;
Widget unit_pix_canvas;
Widget unit_below_canvas[MAX_NUM_UNITS_BELOW];
Widget main_vpane;
Pixmap unit_below_pixmap[MAX_NUM_UNITS_BELOW];
Widget more_arrow_label;
Window root_window;

XtInputId x_input_id;
XtIntervalId x_interval_id;
Atom wm_delete_window;

/****************************************************************************
  Called by the tileset code to set the font size that should be used to
  draw the city names and productions.
****************************************************************************/
void set_city_names_font_sizes(int my_city_names_font_size,
			       int my_city_productions_font_size)
{
  freelog(LOG_ERROR, "Unimplemented set_city_names_font_sizes.");
  /* PORTME */
}

#ifdef UNUSED
/**************************************************************************
...
**************************************************************************/
/* This is used below in ui_main(), in commented out code. */
static int myerr(Display *p, XErrorEvent *e)
{
  puts("error");
  return 0;
}
#endif

/**************************************************************************
  Print extra usage information, including one line help on each option,
  to stderr.
**************************************************************************/
static void print_usage(const char *argv0)
{
  /* add client-specific usage information here */
  fc_fprintf(stderr, _("This client has no special command line options\n\n"));

  /* TRANS: No full stop after the URL, could cause confusion. */
  fc_fprintf(stderr, _("Report bugs at %s\n"), BUG_URL);
}

/**************************************************************************
...
**************************************************************************/
static void parse_options(int argc, char **argv)
{
  int i;

  i = 1;
  while (i < argc)
  {
    if (is_option("--help", argv[i]))
    {
      print_usage(argv[0]);
      exit(EXIT_SUCCESS);
    }
    i += 1;
  }
}

/**************************************************************************
...
**************************************************************************/
static Boolean toplevel_work_proc(XtPointer client_data)
{
  /* This will cause the connect dialog to pop-up.
     We do it here so that the main window exists when that happens,
     so that the connect dialog can position itself relative to it. */
  set_client_state(C_S_DISCONNECTED);
  return (True);
}

/**************************************************************************
...
**************************************************************************/
void ui_init(void)
{

}

/****************************************************************************
  Extra initializers for client options.
****************************************************************************/
void gui_options_extra_init(void)
{
  /* Nothing to do. */
}

/**************************************************************************
  Entry point for whole freeciv client program.
**************************************************************************/
int main(int argc, char **argv)
{
  return client_main(argc, argv);
}

/**************************************************************************
  Entry point for GUI specific portion. Called from client_main()
**************************************************************************/
void ui_main(int argc, char *argv[])
{
  int i;
  struct sprite *icon; 

  parse_options(argc, argv);

  /* include later - pain to see the warning at every run */
  XtSetLanguageProc(NULL, NULL, NULL);
  
  toplevel = XtVaAppInitialize(
	       &app_context,               /* Application context */
	       "Freeciv",                  /* application class name */
#ifdef UNUSED
	       cmd_options, XtNumber(cmd_options),
#else
	       NULL, 0,
#endif
	                                   /* command line option list */
	       &argc, argv,                /* command line args */
	       &fallback_resources[1],     /* for missing app-defaults file */
	       XtNallowShellResize, True,
	       NULL);              

  XtGetApplicationResources(toplevel, &appResources, resources,
                            XtNumber(resources), NULL, 0);

/*  XSynchronize(display, 1); 
  XSetErrorHandler(myerr);*/

  if(appResources.version==NULL)  {
    freelog(LOG_FATAL, _("No version number in resources."));
    freelog(LOG_FATAL, _("You probably have an old (circa V1.0)"
			 " Freeciv resource file somewhere."));
    exit(EXIT_FAILURE);
  }

  /* TODO: Use capabilities here instead of version numbers */
  if (0 != strncmp(appResources.version, VERSION_STRING,
		   strlen(appResources.version))) {
    freelog(LOG_FATAL, _("Game version does not match Resource version."));
    freelog(LOG_FATAL, _("Game version: %s - Resource version: %s"), 
	    VERSION_STRING, appResources.version);
    freelog(LOG_FATAL, _("You might have an old Freeciv resourcefile"
			 " in /usr/lib/X11/app-defaults"));
    exit(EXIT_FAILURE);
  }
  
  if(!appResources.gotAppDefFile) {
    freelog(LOG_NORMAL, _("Using fallback resources - which is OK"));
  }

  display = XtDisplay(toplevel);
  screen_number=XScreenNumberOfScreen(XtScreen(toplevel));
  display_depth=DefaultDepth(display, screen_number);
  root_window=DefaultRootWindow(display);

  display_color_type=get_visual(); 
  
  if(display_color_type!=COLOR_DISPLAY) {
    freelog(LOG_FATAL, _("Only color displays are supported for now..."));
    /*    exit(EXIT_FAILURE); */
  }

  {
    XGCValues values;
    char **missing_charset_list_return;
    int missing_charset_count_return;
    char *def_string_return;
    char *city_names_font, *city_productions_font_name;

    values.graphics_exposures = False;
    civ_gc = XCreateGC(display, root_window, GCGraphicsExposures, &values);

    city_names_font = mystrdup("-*-*-*-*-*-*-14-*");

    city_productions_font_name = mystrdup("-*-*-*-*-*-*-14-*");

    main_font_set = XCreateFontSet(display, city_names_font,
	&missing_charset_list_return,
	&missing_charset_count_return,
	&def_string_return);
    if (!main_font_set) {
      freelog(LOG_FATAL, _("Unable to open fontset: %s"),
	      city_names_font);
      freelog(LOG_FATAL,
	      _("Doing 'xset fp rehash' may temporarily solve a problem."));
      exit(EXIT_FAILURE);
    }
    for (i = 0; i < missing_charset_count_return; i++) {
      freelog(LOG_ERROR, _("Font for charset %s is lacking"),
	      missing_charset_list_return[i]);
    }
    values.foreground = get_color(tileset, COLOR_MAPVIEW_CITYTEXT)->color.pixel;
    values.background = get_color(tileset, COLOR_MAPVIEW_UNKNOWN)->color.pixel;
    font_gc= XCreateGC(display, root_window, 
		       GCForeground|GCBackground|GCGraphicsExposures, 
		       &values);

    prod_font_set = XCreateFontSet(display, city_productions_font_name,
	&missing_charset_list_return,
	&missing_charset_count_return,
	&def_string_return);
    if (!prod_font_set) {
      freelog(LOG_FATAL, _("Unable to open fontset: %s"),
	      city_productions_font_name);
      freelog(LOG_FATAL,
	      _("Doing 'xset fp rehash' may temporarily solve a problem."));
      exit(EXIT_FAILURE);
    }
    for (i = 0; i < missing_charset_count_return; i++) {
      freelog(LOG_ERROR, _("Font for charset %s is lacking"),
	      missing_charset_list_return[i]);
    }
    values.foreground = get_color(tileset, COLOR_MAPVIEW_CITYTEXT)->color.pixel;
    values.background = get_color(tileset, COLOR_MAPVIEW_UNKNOWN)->color.pixel;
    prod_font_gc= XCreateGC(display, root_window,
			    GCForeground|GCBackground|GCGraphicsExposures,
			    &values);

    values.line_width = BORDER_WIDTH;
    values.line_style = LineOnOffDash;
    values.cap_style = CapNotLast;
    values.join_style = JoinMiter;
    values.fill_style = FillSolid;
    border_line_gc = XCreateGC(display, root_window,
			       GCGraphicsExposures|GCLineWidth|GCLineStyle
			       |GCCapStyle|GCJoinStyle|GCFillStyle, &values);

    values.foreground = 0;
    values.background = 0;
    fill_bg_gc= XCreateGC(display, root_window, 
			  GCForeground|GCBackground|GCGraphicsExposures,
			  &values);

    values.fill_style=FillStippled;
    fill_tile_gc= XCreateGC(display, root_window, 
    			    GCForeground|GCBackground|GCFillStyle|GCGraphicsExposures,
			    &values);
  }

  {
    char d1[]={0x03,0x0c,0x03,0x0c};
    char d2[]={0x08,0x02,0x08,0x02};
    gray50 = XCreateBitmapFromData(display, root_window, d1, 4, 4);
    gray25 = XCreateBitmapFromData(display, root_window, d2, 4, 4);
  }
  
  /* 135 below is rough value (could be more intelligent) --dwp */
  num_units_below = 135 / tileset_full_tile_width(tileset);
  num_units_below = MIN(num_units_below,MAX_NUM_UNITS_BELOW);
  num_units_below = MAX(num_units_below,1);
  
  /* do setup_widgets before loading the rest of graphics to ensure that
     setup_widgets() has enough colors available:  (on 256-colour systems)
  */
  setup_widgets();
  tileset_init(tileset);
  tileset_load_tiles(tileset);
  load_intro_gfx();
  load_cursors();

  /* FIXME: what about the mask? */
  icon = get_icon_sprite(tileset, ICON_FREECIV);
  XtVaSetValues(toplevel, XtNiconPixmap, icon->pixmap, NULL);

  XtSetKeyboardFocus(bottom_form, inputline_text);
  XtSetKeyboardFocus(below_menu_form, map_canvas);

  InitializeActions(app_context);

  /* Do this outside setup_widgets() so after tiles are loaded */
  fill_econ_label_pixmaps();
		
  XtAddCallback(map_horizontal_scrollbar, XtNjumpProc, 
		scrollbar_jump_callback, NULL);
  XtAddCallback(map_vertical_scrollbar, XtNjumpProc, 
		scrollbar_jump_callback, NULL);
  XtAddCallback(map_horizontal_scrollbar, XtNscrollProc, 
		scrollbar_scroll_callback, NULL);
  XtAddCallback(map_vertical_scrollbar, XtNscrollProc, 
		scrollbar_scroll_callback, NULL);
  XtAddCallback(turn_done_button, XtNcallback, end_turn_callback, NULL);

  XtAppAddWorkProc(app_context, toplevel_work_proc, NULL);

  XtRealizeWidget(toplevel);

  x_interval_id = XtAppAddTimeOut(app_context, TIMER_INTERVAL,
				  timer_callback, NULL);

  init_mapcanvas_and_overview();

  fill_unit_below_pixmaps();

  set_indicator_icons(client_research_sprite(),
		      client_warming_sprite(),
		      client_cooling_sprite(),
		      client_government_sprite());

  wm_delete_window = XInternAtom(XtDisplay(toplevel), "WM_DELETE_WINDOW", 0);
  XSetWMProtocols(display, XtWindow(toplevel), &wm_delete_window, 1);
  XtOverrideTranslations(toplevel,
    XtParseTranslationTable ("<Message>WM_PROTOCOLS: msg-quit-freeciv()"));

  XtSetSensitive(toplevel, FALSE);

  XtAppMainLoop(app_context);
}

/**************************************************************************
  Do any necessary UI-specific cleanup
**************************************************************************/
void ui_exit()
{

}

/**************************************************************************
  Return our GUI type
**************************************************************************/
enum gui_type get_gui_type(void)
{
  return GUI_XAW;
}

/**************************************************************************
  Callack for when user clicks one of the unit icons on left hand side
  (units on same square as current unit).  Use unit_ids[] data and change
  focus to clicked unit.
**************************************************************************/
static void unit_icon_callback(Widget w, XtPointer client_data,
			       XtPointer call_data) 
{
  struct unit *punit;
  int i = (size_t)client_data;

  assert(i>=0 && i<num_units_below);
  if (unit_ids[i] == 0) /* no unit displayed at this place */
    return;
  punit=game_find_unit_by_number(unit_ids[i]);
  if(punit) { /* should always be true at this point */
    if (unit_owner(punit) == client.conn.playing) {
      /* may be non-true if alliance */
      set_unit_focus(punit);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void setup_widgets(void)
{
  long i;
  int econ_label_count=10, econ_label_space=1;

  main_form = XtVaCreateManagedWidget("mainform", formWidgetClass, 
				      toplevel, 
				      NULL);   

  menu_form = XtVaCreateManagedWidget("menuform", formWidgetClass,	
				      main_form,        
				      NULL);	        
  setup_menus(menu_form); 

/*
 main_vpane= XtVaCreateManagedWidget("mainvpane", 
				      panedWidgetClass, 
				      main_form,
				      NULL);
  
  below_menu_form = XtVaCreateManagedWidget("belowmenuform", 
					    formWidgetClass, 
					    main_vpane,
					    NULL);
*/
  below_menu_form = XtVaCreateManagedWidget("belowmenuform", 
					    formWidgetClass, 
					    main_form,
					    NULL);

  left_column_form = XtVaCreateManagedWidget("leftcolumnform", 
					     formWidgetClass, 
					     below_menu_form, 
					     NULL);

  map_form = XtVaCreateManagedWidget("mapform", 
				     formWidgetClass, 
				     below_menu_form, 
				     NULL);

  bottom_form = XtVaCreateManagedWidget("bottomform", 
					formWidgetClass, 
					/*main_vpane,*/ main_form, 
					NULL);
  
  overview_canvas = XtVaCreateManagedWidget("overviewcanvas", 
					    xfwfcanvasWidgetClass,
					    left_column_form,
					    "exposeProc", 
					    (XtArgVal)overview_canvas_expose,
					    "exposeProcData", 
					    (XtArgVal)NULL,
					    NULL);
  
  info_command = XtVaCreateManagedWidget("infocommand", 
				       commandWidgetClass, 
				       left_column_form, 
				       XtNfromVert, 
				       (XtArgVal)overview_canvas,
				       NULL);   


  /* Don't put the citizens in here yet because not loaded yet */
  for(i=0;i<econ_label_count;i++)  {
    econ_label[i] = XtVaCreateManagedWidget("econlabels",
					    commandWidgetClass,
					    left_column_form,
					    XtNwidth, tileset_small_sprite_width(tileset),
					    XtNheight, tileset_small_sprite_height(tileset),
					    i?XtNfromHoriz:NULL, 
					    i?econ_label[i-1]:NULL,
					    XtNhorizDistance, econ_label_space,
					    NULL);  
  }
  
  bulb_label = XtVaCreateManagedWidget("bulblabel", 
				       labelWidgetClass,
				       left_column_form,
				       NULL);

  sun_label = XtVaCreateManagedWidget("sunlabel", 
				      labelWidgetClass, 
				      left_column_form,
				      NULL);

  flake_label = XtVaCreateManagedWidget("flakelabel", 
					labelWidgetClass, 
					left_column_form,
					NULL);

  government_label = XtVaCreateManagedWidget("governmentlabel", 
					    labelWidgetClass, 
					    left_column_form,
					    NULL);

  timeout_label = XtVaCreateManagedWidget("timeoutlabel", 
					  labelWidgetClass, 
					  left_column_form,
					  NULL);


  turn_done_button =
    I_LW(XtVaCreateManagedWidget("turndonebutton", 
				 commandWidgetClass,
				 left_column_form,
				 XtNwidth, econ_label_count*
						(tileset_small_sprite_width(tileset)+econ_label_space),
				 NULL));

  
  unit_info_label = XtVaCreateManagedWidget("unitinfolabel", 
					    labelWidgetClass, 
					    left_column_form, 
					    NULL);

  unit_pix_canvas = XtVaCreateManagedWidget("unitpixcanvas", 
					   pixcommWidgetClass,
					   left_column_form, 
					   XtNwidth, tileset_full_tile_width(tileset),
					   XtNheight, tileset_full_tile_height(tileset),
					   NULL);

  for(i=0; i<num_units_below; i++) {
    char unit_below_name[32];
    my_snprintf(unit_below_name, sizeof(unit_below_name),
		"unitbelowcanvas%ld", i);
    unit_below_canvas[i] = XtVaCreateManagedWidget(unit_below_name,
						   pixcommWidgetClass,
						   left_column_form, 
						   XtNwidth,
						   tileset_full_tile_width(tileset),
						   XtNheight,
						   tileset_full_tile_height(tileset),
						   NULL);
    XtAddCallback(unit_below_canvas[i], XtNcallback, unit_icon_callback,
		  (XtPointer)i);  
  }

  more_arrow_label =
    XtVaCreateManagedWidget("morearrowlabel", 
			    labelWidgetClass,
			    left_column_form,
			    XtNfromHoriz,
			    (XtArgVal)unit_below_canvas[num_units_below-1],
			    NULL);

  map_vertical_scrollbar = XtVaCreateManagedWidget("mapvertiscrbar", 
						   scrollbarWidgetClass, 
						   map_form,
						   NULL);

  map_canvas = XtVaCreateManagedWidget("mapcanvas", 
				       xfwfcanvasWidgetClass,
				       map_form,
				       "exposeProc", 
				       (XtArgVal)map_canvas_expose,
				       "exposeProcData", 
				       (XtArgVal)NULL,
				       NULL);

  map_horizontal_scrollbar = XtVaCreateManagedWidget("maphorizscrbar", 
						     scrollbarWidgetClass, 
						     map_form,
						     NULL);



  outputwindow_text= I_SW(XtVaCreateManagedWidget("outputwindowtext", 
						  asciiTextWidgetClass, 
						  bottom_form,
						  NULL));


  inputline_text= XtVaCreateManagedWidget("inputlinetext", 
					  asciiTextWidgetClass, 
					  bottom_form,
					  NULL);

}

/**************************************************************************
...
**************************************************************************/
void xaw_ui_exit(void)
{
  tileset_free_tiles(tileset);
  client_exit();
}

/**************************************************************************
...
**************************************************************************/
void main_show_info_popup(XEvent *event)
{
  XButtonEvent *ev = (XButtonEvent *)event;

  if (ev->button == Button1) {
    Widget  p;
    Position x, y;
    Dimension w, h;

    p = XtCreatePopupShell("popupinfo", 
			   overrideShellWidgetClass, 
			   info_command, NULL, 0);

    XtAddCallback(p, XtNpopdownCallback, destroy_me_callback, NULL);

    XtVaCreateManagedWidget("fullinfopopuplabel",
			    labelWidgetClass,
			    p,
			    XtNlabel, get_info_label_text_popup(),
			    NULL);

    XtRealizeWidget(p);

    XtVaGetValues(p, XtNwidth, &w, XtNheight, &h,  NULL);
    XtTranslateCoords(info_command, ev->x, ev->y, &x, &y);
    XtVaSetValues(p, XtNx, MAX(0, x - w / 2), XtNy, MAX(0, y - h / 2), NULL);
    XtPopupSpringLoaded(p);
  }
}

/**************************************************************************
 Update the connected users list at pregame state.
**************************************************************************/
void update_conn_list_dialog(void)
{
  /* PORTME */
  update_start_page();
}

/**************************************************************************
...
**************************************************************************/
void sound_bell(void)
{
  XBell(display, 100);
}

/**************************************************************************
...
**************************************************************************/
static void get_net_input(XtPointer client_data, int *fid, XtInputId *id)
{
  input_from_server(*fid);
}

/**************************************************************************
...
**************************************************************************/
static void set_wait_for_writable_socket(struct connection *pc,
					 bool socket_writable)
{
  static bool previous_state = FALSE;

  if (previous_state == socket_writable)
    return;
  freelog(LOG_DEBUG, "set_wait_for_writable_socket(%d)", socket_writable);
  XtRemoveInput(x_input_id);
  x_input_id = XtAppAddInput(app_context, client.conn.sock,
			     (XtPointer) (XtInputReadMask |
					  (socket_writable ?
					   XtInputWriteMask : 0) |
					  XtInputExceptMask),
			     (XtInputCallbackProc) get_net_input, NULL);
  previous_state = socket_writable;
}

/**************************************************************************
 This function is called after the client succesfully
 has connected to the server
**************************************************************************/
void add_net_input(int sock)
{
  x_input_id = XtAppAddInput(app_context, sock,
			     (XtPointer) (XtInputReadMask |
					  XtInputExceptMask),
			     (XtInputCallbackProc) get_net_input, NULL);
  client.conn.notify_of_writable_data = set_wait_for_writable_socket;
}

/**************************************************************************
 This function is called if the client disconnects
 from the server
**************************************************************************/
void remove_net_input(void)
{
  XtRemoveInput(x_input_id);
  XUndefineCursor(display, XtWindow(map_canvas));
}

/**************************************************************************
  Called to monitor a GGZ socket.
**************************************************************************/
void add_ggz_input(int sock)
{
  /* PORTME */
}

/**************************************************************************
  Called on disconnection to remove monitoring on the GGZ socket.  Only
  call this if we're actually in GGZ mode.
**************************************************************************/
void remove_ggz_input(void)
{
  /* PORTME */
}

/**************************************************************************
...
**************************************************************************/
void end_turn_callback(Widget w, XtPointer client_data, XtPointer call_data)
{ 
  XtSetSensitive(turn_done_button, FALSE);

  user_ended_turn();
}

/**************************************************************************
...
**************************************************************************/
void timer_callback(XtPointer client_data, XtIntervalId * id)
{
  int msec = real_timer_callback() * 1000;

  x_interval_id = XtAppAddTimeOut(app_context, msec,
				  timer_callback, NULL);
}

/**************************************************************************
  Set one of the unit icons in information area based on punit.
  Use punit==NULL to clear icon.
  Index 'idx' is -1 for "active unit", or 0 to (num_units_below-1) for
  units below.  Also updates unit_ids[idx] for idx>=0.
**************************************************************************/
void set_unit_icon(int idx, struct unit *punit)
{
  Widget w;
  
  assert(idx>=-1 && idx<num_units_below);
  if (idx == -1) {
    w = unit_pix_canvas;
  } else {
    w = unit_below_canvas[idx];
    unit_ids[idx] = punit ? punit->id : 0;
  }
  
  XawPixcommClear(w);
  if (punit) {
    struct canvas store = {XawPixcommPixmap(w)};

    put_unit(punit, &store, 0, 0);
    xaw_expose_now(w);
  }
}

/**************************************************************************
  Set the "more arrow" for the unit icons to on(1) or off(0).
  Maintains a static record of current state to avoid unnecessary redraws.
  Note initial state should match initial gui setup (off).
**************************************************************************/
void set_unit_icons_more_arrow(bool onoff)
{
  static bool showing = FALSE;

  if (onoff && !showing) {
    /* FIXME: what about the mask? */
    xaw_set_bitmap(more_arrow_label,
		   get_arrow_sprite(tileset, ARROW_RIGHT)->pixmap);
    showing = TRUE;
  }
  else if(!onoff && showing) {
    xaw_set_bitmap(more_arrow_label, None);
    showing = FALSE;
  }
}

struct callback {
  void (*callback)(void *data);
  void *data;
};

/****************************************************************************
  A wrapper for the callback called through add_idle_callback.
****************************************************************************/
static Boolean idle_callback_wrapper(XtPointer data)
{
  struct callback *cb = data;

  (cb->callback)(cb->data);
  free(cb);
  /* return True if we want to remove WorkProc */
  return True;
}

/****************************************************************************
  Enqueue a callback to be called during an idle moment.  The 'callback'
  function should be called sometimes soon, and passed the 'data' pointer
  as its data.
****************************************************************************/
void add_idle_callback(void (callback)(void *), void *data)
{
  struct callback *cb = fc_malloc(sizeof(*cb));

  cb->callback = callback;
  cb->data = data;
  XtAppAddWorkProc(app_context, idle_callback_wrapper, cb);
}

/**************************************************************************
  Called to fill econ_label pixmaps (showing tax/lux/sci rates).

  It may be called again if the tileset changes.
**************************************************************************/
void fill_econ_label_pixmaps(void)
{
  int i;
  int econ_label_count = 10;

  for(i = 0; i < econ_label_count; i++) {
    struct sprite *s = i < 5 ? get_tax_sprite(tileset, O_SCIENCE)
			     : get_tax_sprite(tileset, O_GOLD);

    XtVaSetValues(econ_label[i], XtNbitmap,
		  s->pixmap, NULL);
    XtAddCallback(econ_label[i], XtNcallback, taxrates_callback,
		  INT_TO_XTPOINTER(i));
  }
}

/**************************************************************************
  Called to fill unit_below pixmaps. They are on the left of the
  screen that shows all of the inactive units in the current tile.

  It may be called again if the tileset changes.
**************************************************************************/
void fill_unit_below_pixmaps(void)
{
  long i;

  for (i = 0; i < num_units_below; i++) {
/*    XtVaSetValues(unit_below_canvas[i],
		  XtNwidth, tileset_full_tile_width(tileset),
		  XtNheight, tileset_full_tile_height(tileset),
		  NULL);
*/    unit_below_pixmap[i] = XCreatePixmap(display, XtWindow(overview_canvas),
					 tileset_full_tile_width(tileset),
					 tileset_full_tile_height(tileset),
					 display_depth);
  }
}

/**************************************************************************
  Called when the tileset is changed to reset indicators pixmaps.
**************************************************************************/
void reset_econ_label_pixmaps(void)
{
  fill_econ_label_pixmaps();
}

/**************************************************************************
  Called when the tileset is changed to reset unit pixmaps.
**************************************************************************/
void reset_unit_below_pixmaps(void)
{
  long i;

  for (i = 0; i < num_units_below; i++) {
    XFreePixmap(display, unit_below_pixmap[i]);
  }
  xaw_set_bitmap(more_arrow_label, None);
  fill_unit_below_pixmaps();

  set_unit_icons_more_arrow(FALSE);
  if (get_num_units_in_focus() == 1) {
    set_unit_icon(-1, head_of_units_in_focus());
  } else {
    set_unit_icon(-1, NULL);
  }
  update_unit_pix_label(get_units_in_focus());
}

/****************************************************************************
  Assigns focus units to given battlegroup.
****************************************************************************/
void assign_battlegroup(int battlegroup)
{
  key_unit_assign_battlegroup(battlegroup, FALSE);
}

/****************************************************************************
  Brings given battlegroup into focus.
****************************************************************************/
void select_battlegroup(int battlegroup)
{
  key_unit_select_battlegroup(battlegroup, FALSE);
}

/****************************************************************************
  Adds focus units to given battlegroup
  (or removes unit from battlegoup if battlegroup is the same that unit has).
****************************************************************************/
void add_unit_to_battlegroup(int battlegroup)
{
  if (NULL != client.conn.playing && can_client_issue_orders()) {
    struct unit *punit;

    punit = head_of_units_in_focus();
    if (punit && punit->battlegroup == battlegroup) {
      /* If top unit already in the same battlegroup, detach it */
      unit_change_battlegroup(punit, BATTLEGROUP_NONE);
      refresh_unit_mapcanvas(punit, punit->tile, TRUE, FALSE);
    } else {
      key_unit_assign_battlegroup(battlegroup, TRUE);
    }
  }
}

/****************************************************************************
  Stub for editor function
****************************************************************************/
void editgui_tileset_changed(void)
{}

/****************************************************************************
  Stub for editor function
****************************************************************************/
void editgui_refresh(void)
{}

/****************************************************************************
  Stub for editor function
****************************************************************************/
void editgui_popup_properties(const struct tile_list *tiles, int objtype)
{}

/****************************************************************************
  Stub for editor function
****************************************************************************/
void editgui_notify_object_changed(int objtype, int object_id, bool remove)
{}

/****************************************************************************
  Stub for editor function
****************************************************************************/
void editgui_notify_object_created(int tag, int id)
{}

/****************************************************************************
  Stub for ggz function
****************************************************************************/
void gui_ggz_embed_leave_table(void)
{}

/****************************************************************************
  Stub for ggz function
****************************************************************************/
void gui_ggz_embed_ensure_server(void)
{}
