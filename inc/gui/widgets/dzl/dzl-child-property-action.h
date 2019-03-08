/* dzl-child-property-action.h
 *
 * Copyright (C) 2017 Christian Hergert <chergert@redhat.com>
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

#ifndef DZL_CHILD_PROPERTY_ACTION_H
#define DZL_CHILD_PROPERTY_ACTION_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DZL_TYPE_CHILD_PROPERTY_ACTION  (dzl_child_property_action_get_type())

G_DECLARE_FINAL_TYPE (DzlChildPropertyAction, dzl_child_property_action,  DZL, CHILD_PROPERTY_ACTION, GObject)

GAction *dzl_child_property_action_new (const gchar  *name,
                                        GtkContainer *container,
                                        GtkWidget    *child,
                                        const gchar  *child_property_name);

G_END_DECLS

#endif /* DZL_CHILD_PROPERTY_ACTION_H */
