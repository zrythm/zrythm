/* dzl-dock-stack.c
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
#include "gui/widgets/dzl/dzl-dock-stack.h"
#include "gui/widgets/dzl/dzl-dock-widget.h"
#include "gui/widgets/dzl/dzl-tab-private.h"
#include "gui/widgets/dzl/dzl-tab-strip.h"
#include "gui/widgets/dzl/dzl-util-private.h"

typedef struct
{
  GtkStack         *stack;
  DzlTabStrip      *tab_strip;
  GtkButton        *pinned_button;
  GtkPositionType   edge : 2;
  DzlTabStyle       style : 2;
} DzlDockStackPrivate;

static void dzl_dock_stack_init_dock_item_iface (DzlDockItemInterface *iface);

G_DEFINE_TYPE_EXTENDED (DzlDockStack, dzl_dock_stack, GTK_TYPE_BOX, 0,
                        G_ADD_PRIVATE (DzlDockStack)
                        G_IMPLEMENT_INTERFACE (DZL_TYPE_DOCK_ITEM,
                                               dzl_dock_stack_init_dock_item_iface))

enum {
  PROP_0,
  PROP_EDGE,
  PROP_SHOW_PINNED_BUTTON,
  PROP_STYLE,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
dzl_dock_stack_add (GtkContainer *container,
                    GtkWidget    *widget)
{
  DzlDockStack *self = (DzlDockStack *)container;
  DzlDockStackPrivate *priv = dzl_dock_stack_get_instance_private (self);
  g_autofree gchar *icon_name = NULL;
  g_autofree gchar *title = NULL;

  g_warn_if_fail (DZL_IS_DOCK_STACK (self));

  if (DZL_IS_DOCK_ITEM (widget))
    {
      title = dzl_dock_item_get_title (DZL_DOCK_ITEM (widget));
      icon_name = dzl_dock_item_get_icon_name (DZL_DOCK_ITEM (widget));
    }

  gtk_container_add_with_properties (GTK_CONTAINER (priv->stack), widget,
                                     "icon-name", icon_name,
                                     "title", title,
                                     NULL);

  if (DZL_IS_DOCK_ITEM (widget))
    dzl_dock_item_adopt (DZL_DOCK_ITEM (self), DZL_DOCK_ITEM (widget));
}

static void
dzl_dock_stack_grab_focus (GtkWidget *widget)
{
  DzlDockStack *self = (DzlDockStack *)widget;
  DzlDockStackPrivate *priv = dzl_dock_stack_get_instance_private (self);
  GtkWidget *child;

  g_warn_if_fail (DZL_IS_DOCK_STACK (self));

  child = gtk_stack_get_visible_child (priv->stack);

  if (child != NULL)
    gtk_widget_grab_focus (GTK_WIDGET (priv->stack));
  else
    GTK_WIDGET_CLASS (dzl_dock_stack_parent_class)->grab_focus (widget);
}

static void
dzl_dock_stack_notify_visible_child_cb (DzlDockStack *self,
                                        GParamSpec   *pspec,
                                        GtkStack     *stack)
{
  GtkWidget *visible_child;

  g_warn_if_fail (DZL_IS_DOCK_STACK (self));
  g_warn_if_fail (GTK_IS_STACK (stack));

  if (gtk_widget_in_destruction (GTK_WIDGET (self)) ||
      gtk_widget_in_destruction (GTK_WIDGET (stack)))
    return;

  if ((visible_child = gtk_stack_get_visible_child (stack)) && DZL_IS_DOCK_ITEM (visible_child))
    dzl_dock_item_emit_presented (DZL_DOCK_ITEM (visible_child));
}

static void
dzl_dock_stack_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  DzlDockStack *self = DZL_DOCK_STACK (object);

  switch (prop_id)
    {
    case PROP_EDGE:
      g_value_set_enum (value, dzl_dock_stack_get_edge (self));
      break;

    case PROP_SHOW_PINNED_BUTTON:
      g_value_set_boolean (value, dzl_dock_stack_get_show_pinned_button (self));
      break;

    case PROP_STYLE:
      g_value_set_flags (value, dzl_dock_stack_get_style (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_dock_stack_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  DzlDockStack *self = DZL_DOCK_STACK (object);

  switch (prop_id)
    {
    case PROP_EDGE:
      dzl_dock_stack_set_edge (self, g_value_get_enum (value));
      break;

    case PROP_SHOW_PINNED_BUTTON:
      dzl_dock_stack_set_show_pinned_button (self, g_value_get_boolean (value));
      break;

    case PROP_STYLE:
      dzl_dock_stack_set_style (self, g_value_get_flags (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_dock_stack_class_init (DzlDockStackClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->get_property = dzl_dock_stack_get_property;
  object_class->set_property = dzl_dock_stack_set_property;

  widget_class->grab_focus = dzl_dock_stack_grab_focus;

  container_class->add = dzl_dock_stack_add;

  properties [PROP_EDGE] =
    g_param_spec_enum ("edge",
                       "Edge",
                       "The edge for the tab strip",
                       GTK_TYPE_POSITION_TYPE,
                       GTK_POS_TOP,
                       (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_SHOW_PINNED_BUTTON] =
    g_param_spec_boolean ("show-pinned-button",
                          "Show Pinned Button",
                          "Show the pinned button to pin the dock edge",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_STYLE] =
    g_param_spec_flags ("style",
                        "Style",
                        "Style",
                        DZL_TYPE_TAB_STYLE,
                        DZL_TAB_BOTH,
                        (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_css_name (widget_class, "dzldockstack");
}

static void
dzl_dock_stack_init (DzlDockStack *self)
{
  DzlDockStackPrivate *priv = dzl_dock_stack_get_instance_private (self);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);

  priv->style = DZL_TAB_BOTH;
  priv->edge = GTK_POS_TOP;

  /*
   * NOTE: setting a transition for the stack seems to muck up
   *       focus, causing the old-tab to get refocused. So we can't
   *       switch to CROSSFADE just yet.
   */

  priv->stack = g_object_new (GTK_TYPE_STACK,
                              "homogeneous", FALSE,
                              "visible", TRUE,
                              NULL);
  g_signal_connect_object (priv->stack,
                           "notify::visible-child",
                           G_CALLBACK (dzl_dock_stack_notify_visible_child_cb),
                           self,
                           G_CONNECT_SWAPPED);

  priv->tab_strip = g_object_new (DZL_TYPE_TAB_STRIP,
                                  "edge", GTK_POS_TOP,
                                  "stack", priv->stack,
                                  "visible", TRUE,
                                  NULL);

  priv->pinned_button = g_object_new (GTK_TYPE_BUTTON,
                                      "action-name", "panel.pinned",
                                      "child", g_object_new (GTK_TYPE_IMAGE,
                                                             "icon-name", "window-maximize-symbolic",
                                                             "visible", TRUE,
                                                             NULL),
                                      "visible", FALSE,
                                      NULL);

  GTK_CONTAINER_CLASS (dzl_dock_stack_parent_class)->add (GTK_CONTAINER (self),
                                                          GTK_WIDGET (priv->tab_strip));
  GTK_CONTAINER_CLASS (dzl_dock_stack_parent_class)->add (GTK_CONTAINER (self),
                                                          GTK_WIDGET (priv->stack));

  dzl_tab_strip_add_control (priv->tab_strip, GTK_WIDGET (priv->pinned_button));
}

GtkWidget *
dzl_dock_stack_new (void)
{
  return g_object_new (DZL_TYPE_DOCK_STACK, NULL);
}

GtkPositionType
dzl_dock_stack_get_edge (DzlDockStack *self)
{
  DzlDockStackPrivate *priv = dzl_dock_stack_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_DOCK_STACK (self), 0);

  return priv->edge;
}

void
dzl_dock_stack_set_edge (DzlDockStack    *self,
                         GtkPositionType  edge)
{
  DzlDockStackPrivate *priv = dzl_dock_stack_get_instance_private (self);

  g_return_if_fail (DZL_IS_DOCK_STACK (self));
  g_return_if_fail (edge >= 0);
  g_return_if_fail (edge <= 3);

  if (edge != priv->edge)
    {
      priv->edge = edge;

      dzl_tab_strip_set_edge (priv->tab_strip, edge);

      switch (edge)
        {
        case GTK_POS_TOP:
          gtk_orientable_set_orientation (GTK_ORIENTABLE (self),
                                          GTK_ORIENTATION_VERTICAL);
          gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->tab_strip),
                                          GTK_ORIENTATION_HORIZONTAL);
          gtk_container_child_set (GTK_CONTAINER (self), GTK_WIDGET (priv->tab_strip),
                                   "position", 0,
                                   NULL);
          break;

        case GTK_POS_BOTTOM:
          gtk_orientable_set_orientation (GTK_ORIENTABLE (self),
                                          GTK_ORIENTATION_VERTICAL);
          gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->tab_strip),
                                          GTK_ORIENTATION_HORIZONTAL);
          gtk_container_child_set (GTK_CONTAINER (self), GTK_WIDGET (priv->tab_strip),
                                   "position", 1,
                                   NULL);
          break;

        case GTK_POS_LEFT:
          gtk_orientable_set_orientation (GTK_ORIENTABLE (self),
                                          GTK_ORIENTATION_HORIZONTAL);
          gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->tab_strip),
                                          GTK_ORIENTATION_VERTICAL);
          gtk_container_child_set (GTK_CONTAINER (self), GTK_WIDGET (priv->tab_strip),
                                   "position", 0,
                                   NULL);
          break;

        case GTK_POS_RIGHT:
          gtk_orientable_set_orientation (GTK_ORIENTABLE (self),
                                          GTK_ORIENTATION_HORIZONTAL);
          gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->tab_strip),
                                          GTK_ORIENTATION_VERTICAL);
          gtk_container_child_set (GTK_CONTAINER (self), GTK_WIDGET (priv->tab_strip),
                                   "position", 1,
                                   NULL);
          break;

        default:
          g_warn_if_reached ();
        }

      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_EDGE]);
    }
}

static void
dzl_dock_stack_present_child (DzlDockItem *item,
                              DzlDockItem *child)
{
  DzlDockStack *self = (DzlDockStack *)item;
  DzlDockStackPrivate *priv = dzl_dock_stack_get_instance_private (self);

  g_warn_if_fail (DZL_IS_DOCK_STACK (self));
  g_warn_if_fail (DZL_IS_DOCK_ITEM (child));

  gtk_stack_set_visible_child (priv->stack, GTK_WIDGET (child));
}

static gboolean
dzl_dock_stack_get_child_visible (DzlDockItem *item,
                                  DzlDockItem *child)
{
  DzlDockStack *self = (DzlDockStack *)item;
  DzlDockStackPrivate *priv = dzl_dock_stack_get_instance_private (self);
  GtkWidget *visible_child;

  g_warn_if_fail (DZL_IS_DOCK_STACK (self));
  g_warn_if_fail (DZL_IS_DOCK_ITEM (child));

  visible_child = gtk_stack_get_visible_child (priv->stack);

  if (visible_child != NULL)
    return gtk_widget_is_ancestor (GTK_WIDGET (child), visible_child);

  return FALSE;
}

static void
dzl_dock_stack_set_child_visible (DzlDockItem *item,
                                  DzlDockItem *child,
                                  gboolean     child_visible)
{
  DzlDockStack *self = (DzlDockStack *)item;
  DzlDockStackPrivate *priv = dzl_dock_stack_get_instance_private (self);
  GtkWidget *parent;
  GtkWidget *last_parent = (GtkWidget *)child;

  g_warn_if_fail (DZL_IS_DOCK_STACK (self));
  g_warn_if_fail (DZL_IS_DOCK_ITEM (child));

  for (parent = gtk_widget_get_parent (GTK_WIDGET (child));
       parent != NULL;
       last_parent = parent, parent = gtk_widget_get_parent (parent))
    {
      if (parent == (GtkWidget *)priv->stack)
        {
          gtk_stack_set_visible_child (priv->stack, last_parent);
          return;
        }
    }
}

static void
update_tab_controls (GtkWidget *widget,
                     gpointer   unused)
{
  g_warn_if_fail (GTK_IS_WIDGET (widget));

  if (DZL_IS_TAB (widget))
    _dzl_tab_update_controls (DZL_TAB (widget));
}

static void
dzl_dock_stack_update_visibility (DzlDockItem *item)
{
  DzlDockStack *self = (DzlDockStack *)item;
  DzlDockStackPrivate *priv = dzl_dock_stack_get_instance_private (self);

  g_warn_if_fail (DZL_IS_DOCK_STACK (self));

  gtk_container_foreach (GTK_CONTAINER (priv->tab_strip),
                         update_tab_controls,
                         NULL);

  if (!dzl_dock_item_has_widgets (item))
    gtk_widget_hide (GTK_WIDGET (item));
  else
    gtk_widget_show (GTK_WIDGET (item));
}

static void
dzl_dock_stack_release (DzlDockItem *item,
                        DzlDockItem *child)
{
  DzlDockStack *self = (DzlDockStack *)item;
  DzlDockStackPrivate *priv = dzl_dock_stack_get_instance_private (self);

  g_warn_if_fail (DZL_IS_DOCK_STACK (self));
  g_warn_if_fail (DZL_IS_DOCK_ITEM (child));

  gtk_container_remove (GTK_CONTAINER (priv->stack), GTK_WIDGET (child));
}

static void
dzl_dock_stack_init_dock_item_iface (DzlDockItemInterface *iface)
{
  iface->present_child = dzl_dock_stack_present_child;
  iface->get_child_visible = dzl_dock_stack_get_child_visible;
  iface->set_child_visible = dzl_dock_stack_set_child_visible;
  iface->update_visibility = dzl_dock_stack_update_visibility;
  iface->release = dzl_dock_stack_release;
}

gboolean
dzl_dock_stack_get_show_pinned_button (DzlDockStack *self)
{
  DzlDockStackPrivate *priv = dzl_dock_stack_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_DOCK_STACK (self), FALSE);

  return gtk_widget_get_visible (GTK_WIDGET (priv->pinned_button));
}

void
dzl_dock_stack_set_show_pinned_button (DzlDockStack *self,
                                       gboolean      show_pinned_button)
{
  DzlDockStackPrivate *priv = dzl_dock_stack_get_instance_private (self);

  g_return_if_fail (DZL_IS_DOCK_STACK (self));

  show_pinned_button = !!show_pinned_button;

  if (show_pinned_button != gtk_widget_get_visible (GTK_WIDGET (priv->pinned_button)))
    {
      gtk_widget_set_visible (GTK_WIDGET (priv->pinned_button), show_pinned_button);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SHOW_PINNED_BUTTON]);
    }
}

DzlTabStyle
dzl_dock_stack_get_style (DzlDockStack *self)
{
  DzlDockStackPrivate *priv = dzl_dock_stack_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_DOCK_STACK (self), 0);

  return priv->style;
}

void
dzl_dock_stack_set_style (DzlDockStack *self,
                          DzlTabStyle   style)
{
  DzlDockStackPrivate *priv = dzl_dock_stack_get_instance_private (self);

  g_return_if_fail (DZL_IS_DOCK_STACK (self));

  if (priv->style != style)
    {
      priv->style = style;
      dzl_tab_strip_set_style (priv->tab_strip, style);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_STYLE]);
    }
}
