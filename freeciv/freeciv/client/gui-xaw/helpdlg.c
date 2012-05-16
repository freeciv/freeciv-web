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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/SimpleMenu.h> 
#include <X11/Xaw/Command.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Tree.h>

/* common & utility */
#include "city.h"
#include "fcintl.h"
#include "genlist.h"
#include "government.h"
#include "mem.h"
#include "shared.h"
#include "tech.h"
#include "unit.h"
#include "map.h"
#include "support.h"
#include "version.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "dialogs.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdata.h"
#include "tilespec.h"

#include "helpdlg.h"

static Widget help_dialog_shell;
static Widget help_form;
static Widget help_viewport;
static Widget help_list;
static Widget help_text;
static Widget help_close_command;

static Widget help_right_form;
static Widget help_title;

static Widget help_improvement_cost, help_improvement_cost_data;
static Widget help_improvement_upkeep, help_improvement_upkeep_data;
static Widget help_improvement_req, help_improvement_req_data;
static Widget help_wonder_obsolete, help_wonder_obsolete_data;

static Widget help_tech_tree;
static Widget help_tree_viewport;

static Widget help_unit_attack, help_unit_attack_data;
static Widget help_unit_def, help_unit_def_data;
static Widget help_unit_move, help_unit_move_data;
static Widget help_unit_hp, help_unit_hp_data;
static Widget help_unit_fp, help_unit_fp_data;
static Widget help_unit_cost, help_unit_cost_data;
static Widget help_unit_visrange, help_unit_visrange_data;
static Widget help_unit_upkeep, help_unit_upkeep_data;
static Widget help_unit_tile;

static Widget help_terrain_movement_defense, help_terrain_movement_defense_data;
static Widget help_terrain_food_shield_trade, help_terrain_food_shield_trade_data;
static Widget help_terrain_resources;
static Widget help_terrain_road_result_time, help_terrain_road_result_time_data;
static Widget help_terrain_irrigation_result_time, help_terrain_irrigation_result_time_data;
static Widget help_terrain_mining_result_time, help_terrain_mining_result_time_data;
static Widget help_terrain_transform_result_time, help_terrain_transform_result_time_data;


static void create_help_page(enum help_page_type type);

static void help_close_command_callback(Widget w, XtPointer client_data, 
				 XtPointer call_data);
static void help_list_callback(Widget w, XtPointer client_data, 
			         XtPointer call_data);
static void help_tree_node_callback(Widget w, XtPointer client_data, 
			     XtPointer call_data);

static void create_help_dialog(void);
static void help_update_dialog(const struct help_item *pitem);

static void select_help_item(int item);
static void select_help_item_string(const char *item,
				    enum help_page_type htype);

static char *topic_list[1024];

#define TREE_NODE_UNKNOWN_TECH_BG "red"
#define TREE_NODE_KNOWN_TECH_BG "green"
#define TREE_NODE_REACHABLE_TECH_BG "yellow"
#define TREE_NODE_REMOVED_TECH_BG "magenta"


/****************************************************************
...
*****************************************************************/
static void set_title_topic(const struct help_item *pitem)
{
  if (strcmp(pitem->topic, "Freeciv") == 0
      || strcmp(pitem->topic, "About") == 0
      || strcmp(pitem->topic, _("About")) == 0) {
    xaw_set_label(help_title, freeciv_name_version());
  } else {
    xaw_set_label(help_title, pitem->topic);
  }
}


/****************************************************************
...
*****************************************************************/
void popdown_help_dialog(void)
{
  if(help_dialog_shell) {
    XtDestroyWidget(help_dialog_shell);
    help_dialog_shell=0;
  }
}
  
/****************************************************************
...
*****************************************************************/
void popup_help_dialog_typed(const char *item, enum help_page_type htype)
{
  Position x, y;
  Dimension width, height;

  if(!help_dialog_shell) {
    create_help_dialog();
    XtVaGetValues(toplevel, XtNwidth, &width, XtNheight, &height, NULL);
    XtTranslateCoords(toplevel, (Position) width/10, (Position) height/10,
		      &x, &y);
    XtVaSetValues(help_dialog_shell, XtNx, x, XtNy, y, NULL);
  }

  XtPopup(help_dialog_shell, XtGrabNone);

  select_help_item_string(item, htype);
}


/****************************************************************
...
Not sure if this should call _(item) as it does, or whether all
callers of this function should do so themselves... --dwp
*****************************************************************/
void popup_help_dialog_string(const char *item)
{
  popup_help_dialog_typed(_(item), HELP_ANY);
}


/**************************************************************************
...
**************************************************************************/
static void create_help_dialog(void)
{
  int i=0;

  help_items_iterate(pitem) 
    topic_list[i++]=pitem->topic;
  help_items_iterate_end;
  topic_list[i]=0;

  
  help_dialog_shell =
    I_T(XtCreatePopupShell("helpdialog", 
			   topLevelShellWidgetClass,
			   toplevel, NULL, 0));

  help_form = XtVaCreateManagedWidget("helpform", 
				      formWidgetClass, 
				      help_dialog_shell, NULL);
  

  help_viewport = XtVaCreateManagedWidget("helpviewport", 
					  viewportWidgetClass, 
					  help_form, 
					  NULL);
  
  help_list = XtVaCreateManagedWidget("helplist", 
				      listWidgetClass, 
				      help_viewport, 
				      XtNlist, 
				      (XtArgVal)topic_list,
				      NULL);

  
  help_right_form=XtVaCreateManagedWidget("helprightform", 
					  formWidgetClass, 
					  help_form, NULL);
  

  help_title=XtVaCreateManagedWidget("helptitle", 
				     labelWidgetClass, 
				     help_right_form,
				     NULL);
  
    
  help_close_command =
    I_L(XtVaCreateManagedWidget("helpclosecommand", commandWidgetClass,
				help_form, NULL));

  XtAddCallback(help_close_command, XtNcallback, 
		help_close_command_callback, NULL);

  XtAddCallback(help_list, XtNcallback, 
		help_list_callback, NULL);

  XtRealizeWidget(help_dialog_shell);

  XSetWMProtocols(display, XtWindow(help_dialog_shell), 
		  &wm_delete_window, 1);
  XtOverrideTranslations(help_dialog_shell,
	 XtParseTranslationTable("<Message>WM_PROTOCOLS: msg-close-help()"));

  /* create_help_page(HELP_IMPROVEMENT); */
  create_help_page(HELP_TEXT);
}


/**************************************************************************
...
**************************************************************************/
static void create_tech_tree(Widget tree, Widget parent, int tech, int levels)
{
  Widget l;
  int type;
  char *bg="";
  char label[MAX_LEN_NAME+3];
  
  type = (tech==A_LAST) ? TECH_UNKNOWN : player_invention_state(client.conn.playing, tech);
  switch(type) {
    case TECH_UNKNOWN:
      bg=TREE_NODE_UNKNOWN_TECH_BG;
      break;
    case TECH_KNOWN:
      bg=TREE_NODE_KNOWN_TECH_BG;
      break;
    case TECH_PREREQS_KNOWN:
      bg=TREE_NODE_REACHABLE_TECH_BG;
      break;
  }
  
  if(tech==A_LAST ||
     (advance_required(tech, AR_ONE)==A_LAST
     && advance_required(tech, AR_TWO)==A_LAST))  {
    sz_strlcpy(label, _("Removed"));
    bg=TREE_NODE_REMOVED_TECH_BG;
    l=XtVaCreateManagedWidget("treenode", commandWidgetClass, 
			      tree,
			      XtNlabel, label,
			      XtNbackground, bg, NULL);
    XtVaSetValues(l, XtVaTypedArg, XtNbackground, 
		  XtRString, bg, strlen(bg)+1, NULL);
     return;
  }
  
  my_snprintf(label, sizeof(label),
	      "%s:%d", advance_name_translation(advance_by_number(tech)),
	      num_unknown_techs_for_goal(client.conn.playing, tech));

  if(parent) {
    l=XtVaCreateManagedWidget("treenode", 
			      commandWidgetClass, 
			      tree,
			      XtNlabel, label,
			      XtNtreeParent, parent,
			      NULL);
  }
  else {
    l=XtVaCreateManagedWidget("treenode", 
			      commandWidgetClass, 
			      tree,
			      XtNlabel, label,
			      NULL);
  }

  XtAddCallback(l, XtNcallback, help_tree_node_callback,
		INT_TO_XTPOINTER(tech));

  XtVaSetValues(l, XtVaTypedArg, XtNbackground, 
		XtRString, bg, strlen(bg)+1, NULL);

  
  if(--levels>0) {
    if(advance_required(tech, AR_ONE)!=A_NONE)
      create_tech_tree(tree, l, advance_required(tech, AR_ONE), levels);
    if(advance_required(tech, AR_TWO)!=A_NONE)
      create_tech_tree(tree, l, advance_required(tech, AR_TWO), levels);
  }
  
  
}


/****************************************************************
...
*****************************************************************/
static void create_help_page(enum help_page_type type)
{
  Dimension w, h, w2, h2, ay, ah;

  XtVaGetValues(help_right_form, XtNwidth, &w, NULL);
  XtVaGetValues(help_viewport, XtNheight, &h, NULL);
  XtVaGetValues(help_title, XtNwidth, &w2, XtNheight, &h2, NULL);

  XtDestroyWidget(help_right_form);

  help_right_form =
    XtVaCreateManagedWidget("helprightform", 
			    formWidgetClass, 
			    help_form, 
			    XtNwidth, w,
			    XtNheight, h,
			    NULL);


  help_title =
    XtVaCreateManagedWidget("helptitle", 
			    labelWidgetClass, 
			    help_right_form,
			    XtNwidth, w2,
			    NULL);

  help_tree_viewport = 0;
  
  if(type==HELP_TEXT || type==HELP_ANY) {
    help_text =
      XtVaCreateManagedWidget("helptext", 
			      asciiTextWidgetClass, 
			      help_right_form,
			      XtNeditType, XawtextRead,
			      XtNscrollVertical, XawtextScrollAlways, 
			      XtNwidth, w2,
			      XtNheight, h-h2-15,
			      XtNbottom, XawChainBottom,
			      NULL);
  }
  else if(type==HELP_IMPROVEMENT || type==HELP_WONDER) {
    help_text =
      XtVaCreateManagedWidget("helptext", 
			      asciiTextWidgetClass, 
			      help_right_form,
			      XtNeditType, XawtextRead,
			      XtNscrollVertical, XawtextScrollAlways, 
			      XtNwidth, w2,
			      XtNheight, 70,
			      NULL);


    help_improvement_cost =
      I_L(XtVaCreateManagedWidget("helpimprcost", 
				  labelWidgetClass, 
				  help_right_form,
				  NULL));
    help_improvement_cost_data =
      XtVaCreateManagedWidget("helpimprcostdata", 
			      labelWidgetClass, 
			      help_right_form,
			      NULL);

    help_improvement_req =
      I_L(XtVaCreateManagedWidget("helpimprreq", 
				  labelWidgetClass, 
				  help_right_form,
				  NULL));
    help_improvement_req_data =
      XtVaCreateManagedWidget("helpimprreqdata", 
			      labelWidgetClass, 
			      help_right_form,
			      NULL);
    if(type==HELP_IMPROVEMENT) {
      help_improvement_upkeep =
	I_L(XtVaCreateManagedWidget("helpimprupkeep", 
				    labelWidgetClass, 
				    help_right_form,
				    NULL));
      help_improvement_upkeep_data =
	XtVaCreateManagedWidget("helpimprupkeepdata", 
				labelWidgetClass, 
				help_right_form,
				NULL);
    } else {
      help_wonder_obsolete =
	I_L(XtVaCreateManagedWidget("helpwonderobsolete", 
				    labelWidgetClass, 
				    help_right_form,
				    NULL));
      help_wonder_obsolete_data =
	XtVaCreateManagedWidget("helpwonderobsoletedata", 
				labelWidgetClass, 
				help_right_form,
				NULL);
    }

    XtVaGetValues(help_improvement_req, XtNy, &ay, XtNheight, &ah, NULL);

    help_tree_viewport =
      XtVaCreateManagedWidget("helptreeviewport", 
			      viewportWidgetClass, 
			      help_right_form,
			      XtNwidth, w2,
			      XtNheight, MAX(1,h-(ay+ah)-10),
			      NULL);
    help_tech_tree =
      XtVaCreateManagedWidget("helptree", 
			      treeWidgetClass, 
			      help_tree_viewport,
			      NULL);

    XawTreeForceLayout(help_tech_tree);  
  }
  else if(type==HELP_UNIT) {
    help_text =
      XtVaCreateManagedWidget("helptext", 
			      asciiTextWidgetClass, 
			      help_right_form,
			      XtNeditType, XawtextRead,
			      XtNscrollVertical, XawtextScrollAlways, 
			      XtNwidth, w2,
			      XtNheight, 70,
			      NULL);

    help_unit_cost =
      I_L(XtVaCreateManagedWidget("helpunitcost", 
				  labelWidgetClass, 
				  help_right_form,
				  NULL));
    help_unit_cost_data =
      XtVaCreateManagedWidget("helpunitcostdata", 
			      labelWidgetClass, 
			      help_right_form,
			      NULL);
    help_unit_attack =
      I_L(XtVaCreateManagedWidget("helpunitattack", 
				  labelWidgetClass, 
				  help_right_form,
				  NULL));
    help_unit_attack_data =
      XtVaCreateManagedWidget("helpunitattackdata", 
			      labelWidgetClass, 
			      help_right_form,
			      NULL);
    help_unit_def =
      I_L(XtVaCreateManagedWidget("helpunitdef", 
				  labelWidgetClass, 
				  help_right_form,
				  NULL));
    help_unit_def_data =
      XtVaCreateManagedWidget("helpunitdefdata", 
			      labelWidgetClass, 
			      help_right_form,
			      NULL);
    help_unit_move =
      I_L(XtVaCreateManagedWidget("helpunitmove", 
				  labelWidgetClass, 
				  help_right_form,
				  NULL));
    help_unit_move_data =
      XtVaCreateManagedWidget("helpunitmovedata", 
			      labelWidgetClass, 
			      help_right_form,
			      NULL);
    help_unit_tile =
      XtVaCreateManagedWidget("helpunittile",
			      labelWidgetClass,
			      help_right_form,
			      XtNwidth, tileset_full_tile_width(tileset),
			      XtNheight, tileset_full_tile_height(tileset),
			      NULL);  
    XtAddCallback(help_unit_tile,
                  XtNdestroyCallback,free_bitmap_destroy_callback,
		  NULL);
    help_unit_fp =
      I_L(XtVaCreateManagedWidget("helpunitfp", 
				  labelWidgetClass, 
				  help_right_form,
				  NULL));
    help_unit_fp_data =
      XtVaCreateManagedWidget("helpunitfpdata", 
			      labelWidgetClass, 
			      help_right_form,
			      NULL);
    help_unit_hp =
      I_L(XtVaCreateManagedWidget("helpunithp", 
				  labelWidgetClass, 
				  help_right_form,
				  NULL));
    help_unit_hp_data =
      XtVaCreateManagedWidget("helpunithpdata", 
			      labelWidgetClass, 
			      help_right_form,
			      NULL);
    help_unit_visrange =
      I_L(XtVaCreateManagedWidget("helpunitvisrange",
				  labelWidgetClass,
				  help_right_form,
				  NULL));
    help_unit_visrange_data =
      XtVaCreateManagedWidget("helpunitvisrangedata",
			      labelWidgetClass,
			      help_right_form,
			      NULL);
    help_unit_upkeep =
      I_L(XtVaCreateManagedWidget("helpunitupkeep",
				  labelWidgetClass,
				  help_right_form,
				  NULL));
    help_unit_upkeep_data =
      XtVaCreateManagedWidget("helpunitupkeepdata",
			      labelWidgetClass,
			      help_right_form,
			      NULL);
    help_improvement_req =
      I_L(XtVaCreateManagedWidget("helpimprreq", 
				  labelWidgetClass, 
				  help_right_form,
				  XtNfromVert, help_unit_upkeep, 
				  NULL));
     help_improvement_req_data =
       XtVaCreateManagedWidget("helpimprreqdata", 
			       labelWidgetClass, 
			       help_right_form,
			       XtNfromVert, help_unit_upkeep, 
			       NULL);
     help_wonder_obsolete =
       I_L(XtVaCreateManagedWidget("helpwonderobsolete", 
				   labelWidgetClass, 
				   help_right_form,
				   XtNfromVert, help_unit_upkeep, 
				   NULL));
     help_wonder_obsolete_data =
       XtVaCreateManagedWidget("helpwonderobsoletedata", 
			       labelWidgetClass, 
			       help_right_form,
			       XtNfromVert, help_unit_upkeep, 
			       NULL);
     XtVaGetValues(help_improvement_req, XtNy, &ay, XtNheight, &ah, NULL);
     help_tree_viewport =
       XtVaCreateManagedWidget("helptreeviewport", 
			       viewportWidgetClass, 
			       help_right_form,
			       XtNwidth, w2,
			       XtNheight, MAX(1,h-(ay+ah)-10),
			       NULL);
     help_tech_tree =
       XtVaCreateManagedWidget("helptree", 
			       treeWidgetClass, 
			       help_tree_viewport,
			       NULL);
     XawTreeForceLayout(help_tech_tree);
  }
  else if(type==HELP_TECH) {
    help_text =
      XtVaCreateManagedWidget("helptext", 
			      asciiTextWidgetClass, 
			      help_right_form,
			      XtNeditType, XawtextRead,
			      XtNscrollVertical, XawtextScrollAlways, 
			      XtNwidth, w2,
			      XtNheight, 95,
			      NULL);

    XtVaGetValues(help_text, XtNy, &ay, XtNheight, &ah, NULL);
    help_tree_viewport =
      XtVaCreateManagedWidget("helptreeviewport", 
			      viewportWidgetClass, 
			      help_right_form,
			      XtNwidth, w2,
			      XtNheight, MAX(1,h-(ay+ah)-10),
			      XtNfromVert,help_text,
			      NULL);
    help_tech_tree =
      XtVaCreateManagedWidget("helptree", 
			      treeWidgetClass, 
			      help_tree_viewport,
			      NULL);
    XawTreeForceLayout(help_tech_tree);  
  }
  else if(type==HELP_TERRAIN) {
    help_text =
      XtVaCreateManagedWidget
      (
       "helptext",
       asciiTextWidgetClass,
       help_right_form,
       XtNeditType, XawtextRead,
       XtNscrollVertical, XawtextScrollAlways,
       XtNwidth, w2,
       XtNheight, 140,
       NULL);

    help_terrain_movement_defense =
      I_L(XtVaCreateManagedWidget
      (
       "help_terrain_movement_defense",
       labelWidgetClass,
       help_right_form,
       NULL));
    help_terrain_movement_defense_data =
      XtVaCreateManagedWidget
      (
       "help_terrain_movement_defense_data",
       labelWidgetClass,
       help_right_form,
       NULL);

    help_terrain_food_shield_trade =
      I_L(XtVaCreateManagedWidget
      (
       "help_terrain_food_shield_trade",
       labelWidgetClass,
       help_right_form,
       NULL));
    help_terrain_food_shield_trade_data =
      XtVaCreateManagedWidget
      (
       "help_terrain_food_shield_trade_data",
       labelWidgetClass,
       help_right_form,
       NULL);

    help_terrain_resources =
      XtVaCreateManagedWidget
      (
       "help_terrain_resources",
       labelWidgetClass,
       help_right_form,
       NULL);

    help_terrain_road_result_time =
      I_L(XtVaCreateManagedWidget
      (
       "help_terrain_road_result_time",
       labelWidgetClass,
       help_right_form,
       NULL));
    help_terrain_road_result_time_data =
      XtVaCreateManagedWidget
      (
       "help_terrain_road_result_time_data",
       labelWidgetClass,
       help_right_form,
       NULL);

    help_terrain_irrigation_result_time =
      I_L(XtVaCreateManagedWidget
      (
       "help_terrain_irrigation_result_time",
       labelWidgetClass,
       help_right_form,
       NULL));
    help_terrain_irrigation_result_time_data =
      XtVaCreateManagedWidget
      (
       "help_terrain_irrigation_result_time_data",
       labelWidgetClass,
       help_right_form,
       NULL);

    help_terrain_mining_result_time =
      I_L(XtVaCreateManagedWidget
      (
       "help_terrain_mining_result_time",
       labelWidgetClass,
       help_right_form,
       NULL));
    help_terrain_mining_result_time_data =
      XtVaCreateManagedWidget
      (
       "help_terrain_mining_result_time_data",
       labelWidgetClass,
       help_right_form,
       NULL);

    help_terrain_transform_result_time =
      I_L(XtVaCreateManagedWidget
      (
       "help_terrain_transform_result_time",
       labelWidgetClass,
       help_right_form,
       NULL));
    help_terrain_transform_result_time_data =
      XtVaCreateManagedWidget
      (
       "help_terrain_transform_result_time_data",
       labelWidgetClass,
       help_right_form,
       NULL);
  }
}

/**************************************************************************
...
**************************************************************************/
static void help_update_improvement(const struct help_item *pitem,
				    char *title)
{
  char buf[64000];
  struct impr_type *imp = find_improvement_by_translated_name(title);
  
  create_help_page(HELP_IMPROVEMENT);

  if (imp  &&  !is_great_wonder(imp)) {
    char req_buf[512];
    int i;

    sprintf(buf, "%d ", impr_build_shield_cost(imp));
    xaw_set_label(help_improvement_cost_data, buf);
    sprintf(buf, "%d ", imp->upkeep);
    xaw_set_label(help_improvement_upkeep_data, buf);
    /*if (imp->tech_req == A_LAST) {
      xaw_set_label(help_improvement_req_data, _("(Never)"));
    } else {
      xaw_set_label(help_improvement_req_data,
		    advance_name_translation(advance_by_number(imp->tech_req)));
    }*/
    
    /* FIXME: this should show ranges and all the MAX_NUM_REQS reqs. 
     * Currently it's limited to 1 req but this code is partially prepared
     * to be extended.  Remember MAX_NUM_REQS is a compile-time
     * definition. */
    i = 0;
    requirement_vector_iterate(&imp->reqs, preq) {
      xaw_set_label(help_improvement_req_data,
                    universal_name_translation(&preq->source,
                                               req_buf, sizeof(req_buf)));
      i++;
      break;
    } requirement_vector_iterate_end;
/*    create_tech_tree(help_tech_tree, 0,
                     imp->req[0].source.value.advance, 3);
*/
  } else {
    xaw_set_label(help_improvement_cost_data, "0 ");
    xaw_set_label(help_improvement_upkeep_data, "0 ");
    xaw_set_label(help_improvement_req_data, _("(Never)"));
    create_tech_tree(help_tech_tree, 0, A_LAST, 3);
  }
  set_title_topic(pitem);
  helptext_building(buf, sizeof(buf), client.conn.playing, pitem->text, imp);
  XtVaSetValues(help_text, XtNstring, buf, NULL);
}
  
/**************************************************************************
...
**************************************************************************/
static void help_update_wonder(const struct help_item *pitem,
			       char *title)
{
  char buf[64000];
  struct impr_type *imp = find_improvement_by_translated_name(title);

  create_help_page(HELP_WONDER);

  if (imp  &&  is_great_wonder(imp)) {
    char req_buf[512];
    int i;
    struct advance *vap;

    sprintf(buf, "%d ", impr_build_shield_cost(imp));
    xaw_set_label(help_improvement_cost_data, buf);
     /* FIXME: this should show ranges and all the MAX_NUM_REQS reqs. 
      * Currently it's limited to 1 req but this code is partially prepared
      * to be extended.  Remember MAX_NUM_REQS is a compile-time
      * definition. */
    i = 0;
    requirement_vector_iterate(&imp->reqs, preq) {
      xaw_set_label(help_improvement_req_data,
                    universal_name_translation(&preq->source,
                                               req_buf, sizeof(req_buf)));
      i++;
      break;
    } requirement_vector_iterate_end;

    vap = valid_advance(imp->obsolete_by);
    if (vap) {
      xaw_set_label(help_wonder_obsolete_data,
		    advance_name_translation(vap));
    } else {
      xaw_set_label(help_wonder_obsolete_data, _("(Never)"));
    }
/*    create_tech_tree(help_tech_tree, 0, 
                     imp->req[0].source.value.advance, 3);
*/
  } else {
    /* can't find wonder */
    xaw_set_label(help_improvement_cost_data, "0 ");
    xaw_set_label(help_improvement_req_data, _("(Never)"));
    xaw_set_label(help_wonder_obsolete_data, _("None"));
    create_tech_tree(help_tech_tree, 0, advance_count(), 3); 
  }
  set_title_topic(pitem);
  helptext_building(buf, sizeof(buf), client.conn.playing, pitem->text, imp);
  XtVaSetValues(help_text, XtNstring, buf, NULL);
}

/**************************************************************************
...
**************************************************************************/
static void help_update_unit_type(const struct help_item *pitem,
				  char *title)
{
  char buf[64000];
  struct unit_type *punittype = find_unit_type_by_translated_name(title);
  
  create_help_page(HELP_UNIT);
  if (punittype) {
/*    struct unit_type *punittype = utype_by_number(i);*/
    sprintf(buf, "%d ", utype_build_shield_cost(punittype));
    xaw_set_label(help_unit_cost_data, buf);
    sprintf(buf, "%d ", punittype->attack_strength);
    xaw_set_label(help_unit_attack_data, buf);
    sprintf(buf, "%d ", punittype->defense_strength);
    xaw_set_label(help_unit_def_data, buf);
    sprintf(buf, "%d ", punittype->move_rate/3);
    xaw_set_label(help_unit_move_data, buf);
    sprintf(buf, "%d ", punittype->firepower);
    xaw_set_label(help_unit_fp_data, buf);
    sprintf(buf, "%d ", punittype->hp);
    xaw_set_label(help_unit_hp_data, buf);
    sprintf(buf, "%d ", (int)sqrt((double)punittype->vision_radius_sq));
    xaw_set_label(help_unit_visrange_data, buf);
    xaw_set_label(help_unit_upkeep_data,
		  helptext_unit_upkeep_str(punittype));
    if (A_NEVER == punittype->require_advance) {
      xaw_set_label(help_improvement_req_data, _("(Never)"));
    } else {
      xaw_set_label(help_improvement_req_data,
		    advance_name_for_player(client.conn.playing,
				  advance_number(punittype->require_advance)));
    }
    create_tech_tree(help_tech_tree, 0, advance_number(punittype->require_advance), 3);
    if (U_NOT_OBSOLETED == punittype->obsoleted_by) {
      xaw_set_label(help_wonder_obsolete_data, _("None"));
    } else {
      xaw_set_label(help_wonder_obsolete_data,
		    utype_name_translation(punittype->obsoleted_by));
    }
    /* add text for transport_capacity, fuel, and flags: */
    helptext_unit(buf, sizeof(buf), client.conn.playing, pitem->text, punittype);
    XtVaSetValues(help_text, XtNstring, buf, NULL);
  }
  else {
    xaw_set_label(help_unit_cost_data, "0 ");
    xaw_set_label(help_unit_attack_data, "0 ");
    xaw_set_label(help_unit_def_data, "0 ");
    xaw_set_label(help_unit_move_data, "0 ");
    xaw_set_label(help_unit_fp_data, "0 ");
    xaw_set_label(help_unit_hp_data, "0 ");
    xaw_set_label(help_unit_visrange_data, "0 ");
    xaw_set_label(help_improvement_req_data, _("(Never)"));
    create_tech_tree(help_tech_tree, 0, advance_count(), 3);
    xaw_set_label(help_wonder_obsolete_data, _("None"));
    XtVaSetValues(help_text, XtNstring, pitem->text, NULL);
  }
  xaw_set_bitmap(help_unit_tile, create_overlay_unit(punittype));
  set_title_topic(pitem);
}

/**************************************************************************
...
**************************************************************************/
static void help_update_tech(const struct help_item *pitem, char *title)
{
  char buf[4096];
  int i;
  struct advance *padvance = find_advance_by_translated_name(title);

  create_help_page(HELP_TECH);
  set_title_topic(pitem);

  if (padvance  &&  !is_future_tech(i = advance_number(padvance))) {
    create_tech_tree(help_tech_tree, 0, i, 3);
    helptext_advance(buf, sizeof(buf), client.conn.playing, pitem->text, i);

    improvement_iterate(pimprove) {
      /*if (i == j->tech_req) 
	sprintf(buf+strlen(buf), _("Allows %s.\n"),
		improvement_name_translation(j));
      */

       /* FIXME: need a more general mechanism for this, since this
        * helptext needs to be shown in all possible req source types. */
      requirement_vector_iterate(&pimprove->reqs, preq) {
	if (VUT_IMPROVEMENT == preq->source.kind
	    && preq->source.value.building == pimprove) {
	  sprintf(buf+strlen(buf), _("Allows %s.\n"),
		  improvement_name_translation(pimprove));
        }
      } requirement_vector_iterate_end;
      if (padvance == pimprove->obsolete_by)
	sprintf(buf+strlen(buf), _("Obsoletes %s.\n"),
		improvement_name_translation(pimprove));
    } improvement_iterate_end;

    unit_type_iterate(punittype) {
      if (padvance != punittype->require_advance) {
	continue;
      }
      sprintf(buf + strlen(buf), _("Allows %s.\n"),
	      utype_name_translation(punittype));
    } unit_type_iterate_end;

    advance_iterate(A_NONE, ptest) {
      if (padvance == advance_requires(ptest, AR_ONE)) {
	if (advance_by_number(A_NONE) == advance_requires(ptest, AR_TWO))
	  sprintf(buf+strlen(buf), _("Allows %s.\n"), 
		  advance_name_translation(ptest));
	else
	  sprintf(buf+strlen(buf), _("Allows %s (with %s).\n"), 
		  advance_name_translation(ptest),
		  advance_name_translation(advance_requires(ptest, AR_TWO)));
      }
      if (padvance == advance_requires(ptest, AR_TWO)) {
	sprintf(buf+strlen(buf), _("Allows %s (with %s).\n"), 
		advance_name_translation(ptest),
		advance_name_translation(advance_requires(ptest, AR_ONE)));
      }
    } advance_iterate_end;
    if (strlen(buf)) strcat(buf, "\n");
/*
    if (advances[i].helptext) {
      sprintf(buf+strlen(buf), "%s\n", _(advances[i].helptext));
    }
*/
  } else {
    create_help_page(HELP_TECH);
    create_tech_tree(help_tech_tree, 0, advance_count(), 3);
    strcpy(buf, pitem->text);
  }
  wordwrap_string(buf, 68);
  XtVaSetValues(help_text, XtNstring, buf, NULL);
}

/**************************************************************************
...
**************************************************************************/
static void help_update_terrain(const struct help_item *pitem,
				char *title)
{
  char buf[4096];
  struct terrain *pterrain = find_terrain_by_translated_name(title);

  create_help_page(HELP_TERRAIN);
  set_title_topic(pitem);

  helptext_terrain(buf, sizeof(buf), client.conn.playing, pitem->text, pterrain);
  XtVaSetValues(help_text, XtNstring, buf, NULL);

  if (pterrain) {
    sprintf(buf, "%d/%d.%d",
	    pterrain->movement_cost,
	    (int)((pterrain->defense_bonus + 100) / 100),
	    (pterrain->defense_bonus + 100) % 100 / 10);
    xaw_set_label(help_terrain_movement_defense_data, buf);

    sprintf(buf, "%d/%d/%d",
	    pterrain->output[O_FOOD],
	    pterrain->output[O_SHIELD],
	    pterrain->output[O_TRADE]);
    xaw_set_label(help_terrain_food_shield_trade_data, buf);

    buf[0] = '\0';
    if (*(pterrain->resources)) {
      struct resource **r;

      for (r = pterrain->resources; *r; r++) {
	sprintf (buf + strlen (buf), " %s,", resource_name_translation(*r));
      }
      buf[strlen (buf) - 1] = '.';
    } else {
      /* TRANS: "Resources: (none)". */
      sprintf (buf + strlen (buf), _("(none)"));
    }
    xaw_set_label(help_terrain_resources, buf);

    if (pterrain->road_trade_incr > 0) {
      sprintf(buf, _("+%d Trade / %d"),
	      pterrain->road_trade_incr,
	      pterrain->road_time);
    } else if (pterrain->road_time > 0) {
      sprintf(buf, _("no extra / %d"), pterrain->road_time);
    } else {
      strcpy(buf, _("n/a"));
    }
    xaw_set_label(help_terrain_road_result_time_data, buf);

    strcpy(buf, _("n/a"));
    if (pterrain->irrigation_result == pterrain) {
      if (pterrain->irrigation_food_incr > 0) {
	sprintf(buf, _("+%d Food / %d"),
		pterrain->irrigation_food_incr,
		pterrain->irrigation_time);
      }
    } else if (pterrain->irrigation_result != T_NONE) {
      sprintf(buf, "%s / %d",
	      terrain_name_translation(pterrain->irrigation_result),
	      pterrain->irrigation_time);
    }
    xaw_set_label(help_terrain_irrigation_result_time_data, buf);

    strcpy(buf, _("n/a"));
    if (pterrain->mining_result == pterrain) {
      if (pterrain->mining_shield_incr > 0) {
	sprintf(buf, _("+%d Res. / %d"),
		pterrain->mining_shield_incr,
		pterrain->mining_time);
      }
    } else if (pterrain->mining_result != T_NONE) {
      sprintf(buf, "%s / %d",
	      terrain_name_translation(pterrain->mining_result),
	      pterrain->mining_time);
    }
    xaw_set_label (help_terrain_mining_result_time_data, buf);

    if (pterrain->transform_result != T_NONE) {
      sprintf(buf, "%s / %d",
	      terrain_name_translation(pterrain->transform_result),
	      pterrain->transform_time);
    } else {
      strcpy(buf, _("n/a"));
    }
    xaw_set_label (help_terrain_transform_result_time_data, buf);
  }
}

/**************************************************************************
  This is currently just a text page, with special text:
**************************************************************************/
static void help_update_government(const struct help_item *pitem,
				   char *title)
{
  char buf[4096];
  struct government *pgovernment = find_government_by_translated_name(title);

  if (!pgovernment) {
    strcat(buf, pitem->text);
  } else {
    helptext_government(buf, sizeof(buf), client.conn.playing, pitem->text, pgovernment);
  }
  create_help_page(HELP_TEXT);
  set_title_topic(pitem);
  XtVaSetValues(help_text, XtNstring, buf, NULL);
}

/**************************************************************************
...
**************************************************************************/
static void help_update_dialog(const struct help_item *pitem)
{
  char *top;

  /* figure out what kind of item is required for pitem ingo */

  for (top = pitem->topic; *top == ' '; top++) {
    /* nothing */
  }

  switch(pitem->type) {
  case HELP_IMPROVEMENT:
    help_update_improvement(pitem, top);
    break;
  case HELP_WONDER:
    help_update_wonder(pitem, top);
    break;
  case HELP_UNIT:
    help_update_unit_type(pitem, top);
    break;
  case HELP_TECH:
    help_update_tech(pitem, top);
    break;
  case HELP_TERRAIN:
    help_update_terrain(pitem, top);
    break;
  case HELP_GOVERNMENT:
    help_update_government(pitem, top);
    break;
  case HELP_TEXT:
  default:
    /* it was a pure text item */ 
    create_help_page(HELP_TEXT);
    set_title_topic(pitem);
    XtVaSetValues(help_text, XtNstring, pitem->text, NULL);
  }

  if (help_tree_viewport) {
    /* Buggy sun athena may require this? --dwp */
    /* And it possibly looks better anyway... */
    XtVaSetValues(help_tree_viewport, XtNforceBars, True, NULL);
  }
}

/**************************************************************************
...
**************************************************************************/
static int help_tree_destroy_children(Widget w)
{
  Widget *children=0;
  Cardinal cnt;
  int did_destroy=0;
  
  XtVaGetValues(help_tech_tree, 
		XtNchildren, &children, 
		XtNnumChildren, &cnt,
		NULL);

  for (; cnt > 0; cnt--, children++) {
    if(XtIsSubclass(*children, commandWidgetClass)) {
      Widget par;
      XtVaGetValues(*children, XtNtreeParent, &par, NULL);
      if(par==w) {
	help_tree_destroy_children(*children);
	XtDestroyWidget(*children);
	did_destroy=1;
      }
    }
  }
  
  return did_destroy;
}


/**************************************************************************
...
**************************************************************************/
static void help_tree_node_callback(Widget w, XtPointer client_data, 
			     XtPointer call_data)
{
  size_t tech=(size_t)client_data;
  
  if(!help_tree_destroy_children(w)) {
    if(advance_required(tech, AR_ONE)!=A_NONE)
      create_tech_tree(help_tech_tree, w, advance_required(tech, AR_ONE), 1);
    if(advance_required(tech, AR_TWO)!=A_NONE)
      create_tech_tree(help_tech_tree, w, advance_required(tech, AR_TWO), 1);
  }
  
}


/**************************************************************************
...
**************************************************************************/
static void help_list_callback(Widget w, XtPointer client_data, 
			XtPointer call_data)
{
  XawListReturnStruct *ret;
  
  ret=XawListShowCurrent(help_list);

  if(ret->list_index!=XAW_LIST_NONE) {
    const struct help_item *pitem=get_help_item(ret->list_index);
    if(pitem)  {
      help_update_dialog(pitem);
      set_title_topic(pitem);
    }
    XawListHighlight(help_list, ret->list_index); 
  }
}

/**************************************************************************
...
**************************************************************************/
static void help_close_command_callback(Widget w, XtPointer client_data, 
				 XtPointer call_data)
{
  XtDestroyWidget(help_dialog_shell);
  help_dialog_shell=0;
}

/****************************************************************
...
*****************************************************************/
void helpdlg_msg_close(Widget w)
{
  help_close_command_callback(w, NULL, NULL);
}

/**************************************************************************
...
**************************************************************************/
static void select_help_item(int item)
{
  int nitems, pos;
  Dimension height;
  
  XtVaGetValues(help_list, XtNnumberStrings, &nitems, NULL);
  XtVaGetValues(help_list, XtNheight, &height, NULL);
  pos= (((double)item)/nitems)*height;
  XawViewportSetCoordinates(help_viewport, 0, pos);
  XawListHighlight(help_list, item); 
}

/****************************************************************
...
*****************************************************************/
static void select_help_item_string(const char *item,
				    enum help_page_type htype)
{
  const struct help_item *pitem;
  int idx;

  pitem = get_help_item_spec(item, htype, &idx);
  if(idx==-1) idx = 0;
  select_help_item(idx);
  help_update_dialog(pitem);
}
