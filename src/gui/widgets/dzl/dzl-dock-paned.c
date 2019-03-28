/* dzl-dock-paned.c
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

#include "gui/widgets/dzl/dzl-dock-item.h"
#include "gui/widgets/dzl/dzl-dock-paned.h"
#include "gui/widgets/dzl/dzl-dock-paned-private.h"
#include "gui/widgets/dzl/dzl-dock-stack.h"

typedef struct
{
  GtkPositionType child_edge : 2;
} DzlDockPanedPrivate;

G_DEFINE_TYPE_EXTENDED (DzlDockPaned, dzl_dock_paned, DZL_TYPE_MULTI_PANED, 0,
                        G_ADD_PRIVATE (DzlDockPaned)
                        G_IMPLEMENT_INTERFACE (DZL_TYPE_DOCK_ITEM, NULL))

static void
dzl_dock_paned_add (GtkContainer *container,
                    GtkWidget    *widget)
{
  DzlDockPaned *self = (DzlDockPaned *)container;
  DzlDockPanedPrivate *priv = dzl_dock_paned_get_instance_private (self);

  g_warn_if_fail (DZL_IS_DOCK_PANED (self));

  if (DZL_IS_DOCK_STACK (widget))
    dzl_dock_stack_set_edge (DZL_DOCK_STACK (widget), priv->child_edge);

  GTK_CONTAINER_CLASS (dzl_dock_paned_parent_class)->add (container, widget);

  if (DZL_IS_DOCK_ITEM (widget))
    {
      dzl_dock_item_adopt (DZL_DOCK_ITEM (self), DZL_DOCK_ITEM (widget));

      /* Always available, so emit presented */
      dzl_dock_item_emit_presented (DZL_DOCK_ITEM (widget));
    }
}

static void
dzl_dock_paned_class_init (DzlDockPanedClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  container_class->add = dzl_dock_paned_add;

  gtk_widget_class_set_css_name (widget_class, "dzldockpaned");
}

static void
dzl_dock_paned_init (DzlDockPaned *self)
{
  DzlDockPanedPrivate *priv = dzl_dock_paned_get_instance_private (self);

  priv->child_edge = GTK_POS_TOP;
}

GtkWidget *
dzl_dock_paned_new (void)
{
  return g_object_new (DZL_TYPE_DOCK_PANED, NULL);
}

static void
dzl_dock_paned_update_child_edge (GtkWidget *widget,
                                  gpointer   user_data)
{
  GtkPositionType child_edge = GPOINTER_TO_INT (user_data);

  g_warn_if_fail (GTK_IS_WIDGET (widget));

  if (DZL_IS_DOCK_STACK (widget))
    dzl_dock_stack_set_edge (DZL_DOCK_STACK (widget), child_edge);
}

void
dzl_dock_paned_set_child_edge (DzlDockPaned    *self,
                               GtkPositionType  child_edge)
{
  DzlDockPanedPrivate *priv = dzl_dock_paned_get_instance_private (self);

  g_return_if_fail (DZL_IS_DOCK_PANED (self));

  if (priv->child_edge != child_edge)
    {
      priv->child_edge = child_edge;

      gtk_container_foreach (GTK_CONTAINER (self),
                             dzl_dock_paned_update_child_edge,
                             GINT_TO_POINTER (child_edge));
    }
}
