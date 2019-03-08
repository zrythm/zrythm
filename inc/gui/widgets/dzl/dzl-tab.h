/* dzl-tab.h
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

#ifndef DZL_TAB_H
#define DZL_TAB_H

#include "gui/widgets/dzl/dzl-dock-types.h"

G_BEGIN_DECLS

const gchar     *dzl_tab_get_icon_name  (DzlTab          *self);
void             dzl_tab_set_icon_name  (DzlTab          *self,
                                         const gchar     *icon_name);
const gchar     *dzl_tab_get_title      (DzlTab          *self);
void             dzl_tab_set_title      (DzlTab          *self,
                                         const gchar     *title);
GtkPositionType  dzl_tab_get_edge       (DzlTab          *self);
void             dzl_tab_set_edge       (DzlTab          *self,
                                         GtkPositionType  edge);
GtkWidget       *dzl_tab_get_widget     (DzlTab          *self);
void             dzl_tab_set_widget     (DzlTab          *self,
                                         GtkWidget       *widget);
gboolean         dzl_tab_get_active     (DzlTab          *self);
void             dzl_tab_set_active     (DzlTab          *self,
                                         gboolean         active);
gboolean         dzl_tab_get_can_close  (DzlTab          *self);
void             dzl_tab_set_can_close  (DzlTab          *self,
                                         gboolean         can_close);
DzlTabStyle      dzl_tab_get_style      (DzlTab          *self);
void             dzl_tab_set_style      (DzlTab          *self,
                                         DzlTabStyle      style);

G_END_DECLS

#endif /* DZL_TAB_H */
