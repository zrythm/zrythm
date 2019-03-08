/* dzl-dock-stack.h
 *
 * Copyright (C) 2016 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DZL_DOCK_STACK_H
#define DZL_DOCK_STACK_H

#include "gui/widgets/dzl/dzl-dock-types.h"

G_BEGIN_DECLS

struct _DzlDockStackClass
{
  GtkBoxClass parent;

  gpointer _reserved1;
  gpointer _reserved2;
  gpointer _reserved3;
  gpointer _reserved4;
};

GtkWidget       *dzl_dock_stack_new                    (void);
GtkPositionType  dzl_dock_stack_get_edge               (DzlDockStack    *self);
void             dzl_dock_stack_set_edge               (DzlDockStack    *self,
                                                        GtkPositionType  edge);
DzlTabStyle      dzl_dock_stack_get_style              (DzlDockStack    *self);
void             dzl_dock_stack_set_style              (DzlDockStack    *self,
                                                        DzlTabStyle      style);
gboolean         dzl_dock_stack_get_show_pinned_button (DzlDockStack    *self);
void             dzl_dock_stack_set_show_pinned_button (DzlDockStack    *self,
                                                        gboolean         show_pinned_button);

G_END_DECLS

#endif /* DZL_DOCK_STACK_H */
