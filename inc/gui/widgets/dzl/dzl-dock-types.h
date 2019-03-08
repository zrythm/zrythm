/* dzl-dock-types.h
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

#ifndef DZL_TYPES_H
#define DZL_TYPES_H

#include <gtk/gtk.h>

#include "gui/widgets/dzl/dzl-bin.h"
#include "gui/widgets/dzl/dzl-multi-paned.h"
#include "gui/widgets/dzl/dzl-dock-revealer.h"

G_BEGIN_DECLS

#define DZL_TYPE_DOCK                   (dzl_dock_get_type ())
#define DZL_TYPE_DOCK_BIN               (dzl_dock_bin_get_type())
#define DZL_TYPE_DOCK_BIN_EDGE          (dzl_dock_bin_edge_get_type())
#define DZL_TYPE_DOCK_ITEM              (dzl_dock_item_get_type())
#define DZL_TYPE_DOCK_MANAGER           (dzl_dock_manager_get_type())
#define DZL_TYPE_DOCK_OVERLAY           (dzl_dock_overlay_get_type())
#define DZL_TYPE_DOCK_OVERLAY_EDGE      (dzl_dock_overlay_edge_get_type())
#define DZL_TYPE_DOCK_PANED             (dzl_dock_paned_get_type())
#define DZL_TYPE_DOCK_STACK             (dzl_dock_stack_get_type())
#define DZL_TYPE_DOCK_TAB_STRIP         (dzl_dock_tab_strip_get_type())
#define DZL_TYPE_DOCK_WIDGET            (dzl_dock_widget_get_type())
#define DZL_TYPE_DOCK_WINDOW            (dzl_dock_window_get_type())
#define DZL_TYPE_TAB                    (dzl_tab_get_type())
#define DZL_TYPE_TAB_STRIP              (dzl_tab_strip_get_type())
#define DZL_TYPE_TAB_STYLE              (dzl_tab_style_get_type())

G_DECLARE_DERIVABLE_TYPE (DzlDockBin,             dzl_dock_bin,               DZL, DOCK_BIN,              GtkContainer)
G_DECLARE_DERIVABLE_TYPE (DzlDockBinEdge,         dzl_dock_bin_edge,          DZL, DOCK_BIN_EDGE,         DzlDockRevealer)
G_DECLARE_DERIVABLE_TYPE (DzlDockManager,         dzl_dock_manager,           DZL, DOCK_MANAGER,          GObject)
G_DECLARE_DERIVABLE_TYPE (DzlDockOverlay,         dzl_dock_overlay,           DZL, DOCK_OVERLAY,          GtkEventBox)
G_DECLARE_DERIVABLE_TYPE (DzlDockPaned,           dzl_dock_paned,             DZL, DOCK_PANED,            DzlMultiPaned)
G_DECLARE_DERIVABLE_TYPE (DzlDockStack,           dzl_dock_stack,             DZL, DOCK_STACK,            GtkBox)
G_DECLARE_DERIVABLE_TYPE (DzlDockWidget,          dzl_dock_widget,            DZL, DOCK_WIDGET,           DzlBin)
G_DECLARE_DERIVABLE_TYPE (DzlDockWindow,          dzl_dock_window,            DZL, DOCK_WINDOW,           GtkWindow)
G_DECLARE_DERIVABLE_TYPE (DzlTabStrip,            dzl_tab_strip,              DZL, TAB_STRIP,             GtkBox)

G_DECLARE_FINAL_TYPE     (DzlTab,                 dzl_tab,                    DZL, TAB,                   DzlBin)
G_DECLARE_FINAL_TYPE     (DzlDockOverlayEdge,     dzl_dock_overlay_edge,      DZL, DOCK_OVERLAY_EDGE,     DzlBin)

G_DECLARE_INTERFACE      (DzlDock,                dzl_dock,                   DZL, DOCK,                  GtkContainer)
G_DECLARE_INTERFACE      (DzlDockItem,            dzl_dock_item,              DZL, DOCK_ITEM,             GtkWidget)

typedef enum
{
  DZL_TAB_TEXT  = 1 << 0,
  DZL_TAB_ICONS = 1 << 1,
  DZL_TAB_BOTH  = (DZL_TAB_TEXT | DZL_TAB_ICONS),
} DzlTabStyle;

GType dzl_tab_style_get_type (void);

G_END_DECLS

#endif /* DZL_TYPES_H */
