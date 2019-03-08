/* dzl-dock-transient-grab.h
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

#ifndef DZL_DOCK_TRANSIENT_GRAB_H
#define DZL_DOCK_TRANSIENT_GRAB_H

#include "gui/widgets/dzl/dzl-dock-item.h"

G_BEGIN_DECLS

#define DZL_TYPE_DOCK_TRANSIENT_GRAB (dzl_dock_transient_grab_get_type())

G_DECLARE_FINAL_TYPE (DzlDockTransientGrab, dzl_dock_transient_grab, DZL, DOCK_TRANSIENT_GRAB, GObject)

DzlDockTransientGrab *dzl_dock_transient_grab_new                    (void);
void                  dzl_dock_transient_grab_add_item               (DzlDockTransientGrab *self,
                                                                      DzlDockItem          *item);
void                  dzl_dock_transient_grab_remove_item            (DzlDockTransientGrab *self,
                                                                      DzlDockItem          *item);
void                  dzl_dock_transient_grab_acquire                (DzlDockTransientGrab *self);
void                  dzl_dock_transient_grab_release                (DzlDockTransientGrab *self);
guint                 dzl_dock_transient_grab_get_timeout            (DzlDockTransientGrab *self);
void                  dzl_dock_transient_grab_set_timeout            (DzlDockTransientGrab *self,
                                                                      guint                 timeout);
gboolean              dzl_dock_transient_grab_contains               (DzlDockTransientGrab *self,
                                                                      DzlDockItem          *item);
gboolean              dzl_dock_transient_grab_is_descendant          (DzlDockTransientGrab *self,
                                                                      GtkWidget            *widget);
void                  dzl_dock_transient_grab_steal_common_ancestors (DzlDockTransientGrab *self,
                                                                      DzlDockTransientGrab *other);
void                  dzl_dock_transient_grab_cancel                 (DzlDockTransientGrab *self);

G_END_DECLS

#endif /* DZL_DOCK_TRANSIENT_GRAB_H */

