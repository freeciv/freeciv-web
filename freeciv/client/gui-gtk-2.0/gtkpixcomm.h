/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __GTK_PIXCOMM_H__
#define __GTK_PIXCOMM_H__


#include <gtk/gtkmisc.h>

#include "sprite.h"


G_BEGIN_DECLS


#define GTK_TYPE_PIXCOMM		 (gtk_pixcomm_get_type ())
#define GTK_PIXCOMM(obj)		 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_PIXCOMM, GtkPixcomm))
#define GTK_PIXCOMM_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_PIXCOMM, GtkPixcommClass))
#define GTK_IS_PIXCOMM(obj)		 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_PIXCOMM))
#define GTK_IS_PIXCOMM_CLASS(klass)	 (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_PIXCOMM))
#define GTK_PIXCOMM_GET_CLASS(obj)	 (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_PIXCOMM, GtkPixcommClass))


typedef struct _GtkPixcomm	GtkPixcomm;
typedef struct _GtkPixcommClass	GtkPixcommClass;

struct _GtkPixcomm
{
  GtkMisc misc;
  
  guint freeze_count;

  gint w, h;
  GArray *actions;

  gboolean is_scaled;
  gdouble scale;
};

struct _GtkPixcommClass
{
  GtkMiscClass parent_class;
};


GType	   gtk_pixcomm_get_type	 (void) G_GNUC_CONST;
GtkWidget *gtk_pixcomm_new	 (gint width, gint height);
void gtk_pixcomm_set_scale(GtkPixcomm *pixcomm, gdouble scale);
void gtk_pixcomm_copyto(GtkPixcomm *pixcomm, struct sprite *src,
			gint x, gint y);
void       gtk_pixcomm_clear	 (GtkPixcomm *pixcomm);
void	   gtk_pixcomm_fill	 (GtkPixcomm *pixcomm, GdkColor *color);

void	   gtk_pixcomm_freeze	 (GtkPixcomm *pixcomm);
void	   gtk_pixcomm_thaw	 (GtkPixcomm *pixcomm);


G_END_DECLS

#endif /* __GTK_PIXCOMM_H__ */

