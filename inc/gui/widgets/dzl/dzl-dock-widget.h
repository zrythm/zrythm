/* dzl-dock-widget.h
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

#ifndef DZL_DOCK_WIDGET_H
#define DZL_DOCK_WIDGET_H

#include "gui/widgets/dzl/dzl-dock-types.h"

G_BEGIN_DECLS

struct _DzlDockWidgetClass
{
  DzlBinClass parent;

  gpointer _reserved1;
  gpointer _reserved2;
  gpointer _reserved3;
  gpointer _reserved4;
  gpointer _reserved5;
  gpointer _reserved6;
  gpointer _reserved7;
  gpointer _reserved8;
};

GtkWidget *dzl_dock_widget_new           (void);
void       dzl_dock_widget_set_title     (DzlDockWidget *self,
                                          const gchar   *title);
void       dzl_dock_widget_set_icon_name (DzlDockWidget *self,
                                          const gchar   *icon_name);

G_END_DECLS

#endif /* DZL_DOCK_WIDGET_H */
