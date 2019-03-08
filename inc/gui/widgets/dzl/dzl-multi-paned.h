/* dzl-multi-paned.h
 *
 * Copyright (C) 2016 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DZL_MULTI_PANED_H
#define DZL_MULTI_PANED_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DZL_TYPE_MULTI_PANED (dzl_multi_paned_get_type())

G_DECLARE_DERIVABLE_TYPE (DzlMultiPaned, dzl_multi_paned, DZL, MULTI_PANED, GtkContainer)

struct _DzlMultiPanedClass
{
  GtkContainerClass parent;

  void (*resize_drag_begin) (DzlMultiPaned *self,
                             GtkWidget     *child);
  void (*resize_drag_end)   (DzlMultiPaned *self,
                             GtkWidget     *child);

  gpointer _reserved1;
  gpointer _reserved2;
  gpointer _reserved3;
  gpointer _reserved4;
  gpointer _reserved5;
  gpointer _reserved6;
  gpointer _reserved7;
  gpointer _reserved8;
};

GtkWidget *dzl_multi_paned_new            (void);
guint      dzl_multi_paned_get_n_children (DzlMultiPaned *self);
GtkWidget *dzl_multi_paned_get_nth_child  (DzlMultiPaned *self,
                                           guint          nth);
GtkWidget *dzl_multi_paned_get_at_point   (DzlMultiPaned *self,
                                           gint           x,
                                           gint           y);

G_END_DECLS

#endif /* DZL_MULTI_PANED_H */
