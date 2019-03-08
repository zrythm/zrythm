/* dzl-dock-paned-private.h
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

#ifndef DZL_DOCK_PANED_PRIVATE_H
#define DZL_DOCK_PANED_PRIVATE_H

#include "gui/widgets/dzl/dzl-dock-paned.h"

G_BEGIN_DECLS

void dzl_dock_paned_set_child_edge (DzlDockPaned    *self,
                                    GtkPositionType  child_edge);

G_END_DECLS

#endif /* DZL_DOCK_PANED_PRIVATE_H */
