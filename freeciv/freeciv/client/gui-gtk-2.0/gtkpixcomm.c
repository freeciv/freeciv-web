/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * Insensitive pixcomm building code by Eckehard Berns from GNOME Stock
 * Copyright (C) 1997, 1998 Free Software Foundation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

/*
 * Based code for GtkPixcomm off GtkImage from the standard GTK+ distribution.
 * This widget will have a built-in X window for capturing "click" events, so
 * that we no longer need to insert it inside a GtkEventBox. -vasc
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>

#include "gui_main.h"
#include "gtkpixcomm.h"


static void	gtk_pixcomm_class_init (GtkPixcommClass *klass);
static void	gtk_pixcomm_init       (GtkPixcomm *pixcomm);
static gboolean gtk_pixcomm_expose     (GtkWidget *widget, GdkEventExpose *ev);
static void	gtk_pixcomm_destroy    (GtkObject *object);
#if 0
static void build_insensitive_pixbuf (GtkPixcomm *pixcomm);
#endif

static GtkMiscClass *parent_class;


enum op_t {
  OP_FILL,
  OP_COPY,
  OP_END
};

struct op {
  enum op_t type;

  /* OP_FILL */
  GdkColor *color;

  /* OP_COPY */
  struct sprite *src;
  gint x, y;
};


GType
gtk_pixcomm_get_type(void)
{
  static GtkType pixcomm_type = 0;

  if (!pixcomm_type) {
    static const GTypeInfo pixcomm_info = {
      sizeof(GtkPixcommClass),
      NULL,		/* base_init */
      NULL,		/* base_finalize */
      (GClassInitFunc) gtk_pixcomm_class_init,
      NULL,		/* class_finalize */
      NULL,		/* class_data */
      sizeof(GtkPixcomm),
      0,		/* n_preallocs */
      (GInstanceInitFunc) gtk_pixcomm_init
    };

    pixcomm_type = g_type_register_static(GTK_TYPE_MISC, "GtkPixcomm",
					  &pixcomm_info, 0);
  }

  return pixcomm_type;
}

static void
gtk_pixcomm_class_init(GtkPixcommClass *klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  object_class->destroy = gtk_pixcomm_destroy;
  widget_class->expose_event = gtk_pixcomm_expose;
}

static void
gtk_pixcomm_init(GtkPixcomm *pixcomm)
{
  GTK_WIDGET_SET_FLAGS(pixcomm, GTK_NO_WINDOW);

  pixcomm->actions = NULL;
  pixcomm->freeze_count = 0;
}

static void
gtk_pixcomm_destroy(GtkObject *object)
{
  GtkPixcomm *p = GTK_PIXCOMM(object);

  g_object_freeze_notify(G_OBJECT(p));
  
  if (p->actions) {
    g_array_free(p->actions, TRUE);
  }
  p->actions = NULL;

  g_object_thaw_notify(G_OBJECT(p));

  if (GTK_OBJECT_CLASS(parent_class)->destroy) {
    (*GTK_OBJECT_CLASS(parent_class)->destroy)(object);
  }
}

GtkWidget*
gtk_pixcomm_new(gint width, gint height)
{
  GtkPixcomm *p;

  p = g_object_new(gtk_pixcomm_get_type(), NULL);
  p->w = width; p->h = height;

  p->is_scaled = FALSE;
  p->scale = 1.0;

  p->actions = g_array_new(FALSE, FALSE, sizeof(struct op));

  GTK_WIDGET(p)->requisition.width = p->w + GTK_MISC(p)->xpad * 2;
  GTK_WIDGET(p)->requisition.height = p->h + GTK_MISC(p)->ypad * 2;

  return GTK_WIDGET(p);
}

/****************************************************************************
  Set the scaling on the pixcomm.  All operations drawn on the pixcomm
  (before or after this function is called) will simply be scaled
  by this amount.
****************************************************************************/
void gtk_pixcomm_set_scale(GtkPixcomm *pixcomm, gdouble scale)
{
  g_return_if_fail(GTK_IS_PIXCOMM(pixcomm));
  g_return_if_fail(scale > 0.0);

  if (scale == 1.0) {
    pixcomm->is_scaled = FALSE;
    pixcomm->scale = 1.0;
  } else {
    pixcomm->is_scaled = TRUE;
    pixcomm->scale = scale;
  }
}

static void
refresh(GtkPixcomm *p)
{
  if (p->freeze_count == 0) {
    gtk_widget_queue_draw(GTK_WIDGET(p));
  }
}


void
gtk_pixcomm_clear(GtkPixcomm *p)
{
  g_return_if_fail(GTK_IS_PIXCOMM(p));

  g_array_set_size(p->actions, 0);
  refresh(p);
}

void
gtk_pixcomm_fill(GtkPixcomm *p, GdkColor *color)
{
  struct op v;

  g_return_if_fail(GTK_IS_PIXCOMM(p));
  g_return_if_fail(color != NULL);

  g_array_set_size(p->actions, 0);

  v.type	= OP_FILL;
  v.color	= color;
  g_array_append_val(p->actions, v);
  refresh(p);
}

void gtk_pixcomm_copyto(GtkPixcomm *p, struct sprite *src, gint x, gint y)
{
  struct op v;

  g_return_if_fail(GTK_IS_PIXCOMM(p));
  g_return_if_fail(src != NULL);

  v.type	= OP_COPY;
  v.src		= src;
  v.x		= x;
  v.y		= y;
  g_array_append_val(p->actions, v);
  refresh(p);
}

static gboolean
gtk_pixcomm_expose(GtkWidget *widget, GdkEventExpose *ev)
{
  g_return_val_if_fail(GTK_IS_PIXCOMM(widget), FALSE);
  g_return_val_if_fail(ev!=NULL, FALSE);

  if (GTK_WIDGET_DRAWABLE(widget)) {
    GtkPixcomm *p;
    GtkMisc *misc;
    gint x, y;
    gfloat xalign;
    guint i;

    p = GTK_PIXCOMM(widget);
    misc = GTK_MISC(widget);

    if (p->actions->len <= 0)
      return FALSE;

    if (gtk_widget_get_direction(widget) == GTK_TEXT_DIR_LTR) {
      xalign = misc->xalign;
    } else {
      xalign = 1.0 - misc->xalign;
    }

    x = floor(widget->allocation.x + misc->xpad
	+ ((widget->allocation.width - widget->requisition.width) *
	  xalign)
	+ 0.5);
    y = floor(widget->allocation.y + misc->ypad 
	+ ((widget->allocation.height - widget->requisition.height) *
	  misc->yalign)
	+ 0.5);

    /* draw! */
    for (i = 0; i < p->actions->len; i++) {
      const struct op *rop = &g_array_index(p->actions, struct op, i);

      switch (rop->type) {
      case OP_FILL:
        gdk_gc_set_foreground(civ_gc, rop->color);
        gdk_draw_rectangle(widget->window, civ_gc, TRUE, x, y, p->w, p->h);
        break;

      case OP_COPY:
	if (p->is_scaled) {
	  int w = rop->src->width * p->scale + 0.5;
	  int h = rop->src->height * p->scale + 0.5;
	  int ox = rop->x * p->scale + 0.5;
	  int oy = rop->y * p->scale + 0.5;
	  GdkPixbuf *pixbuf = sprite_get_pixbuf(rop->src);
	  GdkPixbuf *scaled
	    = gdk_pixbuf_scale_simple(pixbuf, w, h, GDK_INTERP_BILINEAR);

	  gdk_draw_pixbuf(widget->window, civ_gc,
			  scaled, 0, 0, x + ox, y + oy,
			  w, h, GDK_RGB_DITHER_NONE, 0, 0);
	  g_object_unref(scaled);
	} else if (rop->src->pixmap) {
	  if (rop->src->mask) {
	    gdk_gc_set_clip_mask(civ_gc, rop->src->mask);
	    gdk_gc_set_clip_origin(civ_gc, x + rop->x, y + rop->y);

	    gdk_draw_drawable(widget->window, civ_gc,
			      rop->src->pixmap,
			      0, 0,
			      x + rop->x, y + rop->y,
			      rop->src->width, rop->src->height);

	    gdk_gc_set_clip_origin(civ_gc, 0, 0);
	    gdk_gc_set_clip_mask(civ_gc, NULL);
	  } else {
	    gdk_draw_drawable(widget->window, civ_gc,
			      rop->src->pixmap,
			      0, 0,
			      x + rop->x, y + rop->y,
			      rop->src->width, rop->src->height);
	  }
	} else {
	  gdk_draw_pixbuf(widget->window, civ_gc,
			  rop->src->pixbuf,
			  0, 0,
			  x + rop->x, y + rop->y,
			  rop->src->width, rop->src->height,
			  GDK_RGB_DITHER_NONE, 0, 0);
	}
        break;

      default:
        break;
      }
    }
  }
  return FALSE;
}

void
gtk_pixcomm_freeze(GtkPixcomm *p)
{
  g_return_if_fail(GTK_IS_PIXCOMM(p));

  p->freeze_count++;
}

void
gtk_pixcomm_thaw(GtkPixcomm *p)
{
  g_return_if_fail(GTK_IS_PIXCOMM(p));

  if (p->freeze_count > 0) {
    p->freeze_count--;

    refresh(p);
  }
}

#if 0
static void
build_insensitive_pixbuf(GtkPixcomm *pixcomm)
{
  /* gdk_pixbuf_composite_color_simple() */
}
#endif
