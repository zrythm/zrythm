/* dzl-dock-item.h
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

#ifndef DZL_DOCK_ITEM_H
#define DZL_DOCK_ITEM_H

#include "gui/widgets/dzl/dzl-dock-manager.h"

G_BEGIN_DECLS

struct _DzlDockItemInterface
{
  GTypeInterface parent;

  void            (*set_manager)        (DzlDockItem     *self,
                                         DzlDockManager  *manager);
  DzlDockManager *(*get_manager)        (DzlDockItem     *self);
  void            (*manager_set)        (DzlDockItem     *self,
                                         DzlDockManager  *old_manager);
  void            (*present_child)      (DzlDockItem     *self,
                                         DzlDockItem     *child);
  void            (*update_visibility)  (DzlDockItem     *self);
  gboolean        (*get_child_visible)  (DzlDockItem     *self,
                                         DzlDockItem     *child);
  void            (*set_child_visible)  (DzlDockItem     *self,
                                         DzlDockItem     *child,
                                         gboolean         child_visible);
  gchar          *(*get_title)          (DzlDockItem     *self);
  gchar          *(*get_icon_name)      (DzlDockItem     *self);
  gboolean        (*get_can_close)      (DzlDockItem     *self);
  gboolean        (*can_minimize)       (DzlDockItem     *self,
                                         DzlDockItem     *descendant);
  gboolean        (*close)              (DzlDockItem     *self);
  gboolean        (*minimize)           (DzlDockItem     *self,
                                         DzlDockItem     *child,
                                         GtkPositionType *position);
  void            (*release)            (DzlDockItem     *self,
                                         DzlDockItem     *child);
  void            (*presented)          (DzlDockItem     *self);
};

DzlDockManager *dzl_dock_item_get_manager       (DzlDockItem     *self);
void            dzl_dock_item_set_manager       (DzlDockItem     *self,
                                                 DzlDockManager  *manager);
gboolean        dzl_dock_item_adopt             (DzlDockItem     *self,
                                                 DzlDockItem     *child);
void            dzl_dock_item_present           (DzlDockItem     *self);
void            dzl_dock_item_present_child     (DzlDockItem     *self,
                                                 DzlDockItem     *child);
void            dzl_dock_item_update_visibility (DzlDockItem     *self);
gboolean        dzl_dock_item_has_widgets       (DzlDockItem     *self);
gboolean        dzl_dock_item_get_child_visible (DzlDockItem     *self,
                                                 DzlDockItem     *child);
void            dzl_dock_item_set_child_visible (DzlDockItem     *self,
                                                 DzlDockItem     *child,
                                                 gboolean         child_visible);
DzlDockItem    *dzl_dock_item_get_parent        (DzlDockItem     *self);
gchar          *dzl_dock_item_get_title         (DzlDockItem     *self);
gchar          *dzl_dock_item_get_icon_name     (DzlDockItem     *self);
gboolean        dzl_dock_item_get_can_close     (DzlDockItem     *self);
gboolean        dzl_dock_item_get_can_minimize  (DzlDockItem     *self);
gboolean        dzl_dock_item_close             (DzlDockItem     *self);
gboolean        dzl_dock_item_minimize          (DzlDockItem     *self,
                                                 DzlDockItem     *child,
                                                 GtkPositionType *position);
void            dzl_dock_item_release           (DzlDockItem     *self,
                                                 DzlDockItem     *child);
void            dzl_dock_item_emit_presented    (DzlDockItem     *self);
void            _dzl_dock_item_printf           (DzlDockItem     *self);

G_END_DECLS

#endif /* DZL_DOCK_ITEM_H */
