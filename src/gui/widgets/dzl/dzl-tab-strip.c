/* dzl-tab-strip.c
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

#include "gui/widgets/dzl/dzl-dock-item.h"
#include "gui/widgets/dzl/dzl-dock-widget.h"
#include "gui/widgets/dzl/dzl-tab.h"
#include "gui/widgets/dzl/dzl-tab-strip.h"
#include "gui/widgets/dzl/dzl-macros.h"
#include "utils/gtk.h"

typedef struct
{
  GAction         *action;
  GtkStack        *stack;
  GtkPositionType  edge : 2;
  DzlTabStyle      style : 2;
} DzlTabStripPrivate;

static void buildable_iface_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (DzlTabStrip, dzl_tab_strip, GTK_TYPE_BOX,
                         G_ADD_PRIVATE (DzlTabStrip)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, buildable_iface_init))

enum {
  PROP_0,
  PROP_EDGE,
  PROP_STACK,
  PROP_STYLE,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
set_tab_state (GSimpleAction *action,
               GVariant      *state,
               gpointer       user_data)
{
  DzlTabStrip *self = user_data;
  DzlTabStripPrivate *priv = dzl_tab_strip_get_instance_private (self);
  GtkWidget *nth_child;
  DzlTab *tab = NULL;
  GList *list;
  gint stateval;

  g_warn_if_fail (G_IS_SIMPLE_ACTION (action));
  g_warn_if_fail (DZL_IS_TAB_STRIP (self));
  g_warn_if_fail (state != NULL);
  g_warn_if_fail (g_variant_is_of_type (state, G_VARIANT_TYPE_INT32));

  g_simple_action_set_state (action, state);

  stateval = g_variant_get_int32 (state);

  list = gtk_container_get_children (GTK_CONTAINER (priv->stack));
  nth_child = g_list_nth_data (list, stateval);
  g_clear_pointer (&list, g_list_free);

  if (nth_child != NULL)
    {
      tab = g_object_get_data (G_OBJECT (nth_child), "DZL_TAB");
      gtk_stack_set_visible_child (priv->stack, nth_child);
      /*
       * When clicking an active toggle button, we get the state callback but then
       * the toggle button disables the checked state. So ensure it stays on by
       * manually setting the state.
       */
      if (DZL_IS_TAB (tab))
        dzl_tab_set_active (DZL_TAB (tab), TRUE);
    }
}

static void
dzl_tab_strip_update_action_targets (DzlTabStrip *self)
{
  const GList *iter;
  GList *list;
  gint i;

  g_warn_if_fail (DZL_IS_TAB_STRIP (self));

  list = gtk_container_get_children (GTK_CONTAINER (self));

  for (i = 0, iter = list; iter != NULL; iter = iter->next, i++)
    {
      GtkWidget *widget = iter->data;

      /* Ignore controls, and just update tabs */
      if (DZL_IS_TAB (widget))
        gtk_actionable_set_action_target (GTK_ACTIONABLE (widget), "i", i);
    }

  g_list_free (list);
}

static void
dzl_tab_strip_add (GtkContainer *container,
                   GtkWidget    *widget)
{
  DzlTabStrip *self = (DzlTabStrip *)container;
  DzlTabStripPrivate *priv = dzl_tab_strip_get_instance_private (self);

  g_warn_if_fail (DZL_IS_TAB_STRIP (self));
  g_warn_if_fail (GTK_IS_WIDGET (widget));

  if (DZL_IS_TAB (widget))
    dzl_tab_set_edge (DZL_TAB (widget), priv->edge);

  GTK_CONTAINER_CLASS (dzl_tab_strip_parent_class)->add (container, widget);

  dzl_tab_strip_update_action_targets (self);
}

static void
dzl_tab_strip_remove (GtkContainer *container,
                      GtkWidget    *widget)
{
  DzlTabStrip *self = (DzlTabStrip *)container;

  g_warn_if_fail (DZL_IS_TAB_STRIP (self));
  g_warn_if_fail (GTK_IS_WIDGET (widget));

  GTK_CONTAINER_CLASS (dzl_tab_strip_parent_class)->remove (container, widget);

  dzl_tab_strip_update_action_targets (self);
}

static void
dzl_tab_strip_destroy (GtkWidget *widget)
{
  DzlTabStrip *self = (DzlTabStrip *)widget;
  DzlTabStripPrivate *priv = dzl_tab_strip_get_instance_private (self);

  g_warn_if_fail (DZL_IS_TAB_STRIP (self));

  gtk_widget_insert_action_group (GTK_WIDGET (self), "tab-strip", NULL);

  dzl_tab_strip_set_stack (self, NULL);

  g_clear_object (&priv->action);
  g_clear_object (&priv->stack);

  GTK_WIDGET_CLASS (dzl_tab_strip_parent_class)->destroy (widget);
}

static void
dzl_tab_strip_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  DzlTabStrip *self = DZL_TAB_STRIP (object);

  switch (prop_id)
    {
    case PROP_EDGE:
      g_value_set_enum (value, dzl_tab_strip_get_edge (self));
      break;

    case PROP_STACK:
      g_value_set_object (value, dzl_tab_strip_get_stack (self));
      break;

    case PROP_STYLE:
      g_value_set_flags (value, dzl_tab_strip_get_style (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_tab_strip_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  DzlTabStrip *self = DZL_TAB_STRIP (object);

  switch (prop_id)
    {
    case PROP_EDGE:
      dzl_tab_strip_set_edge (self, g_value_get_enum (value));
      break;

    case PROP_STACK:
      dzl_tab_strip_set_stack (self, g_value_get_object (value));
      break;

    case PROP_STYLE:
      dzl_tab_strip_set_style (self, g_value_get_flags (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_tab_strip_class_init (DzlTabStripClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->get_property = dzl_tab_strip_get_property;
  object_class->set_property = dzl_tab_strip_set_property;

  widget_class->destroy = dzl_tab_strip_destroy;

  container_class->add = dzl_tab_strip_add;
  container_class->remove = dzl_tab_strip_remove;

  properties [PROP_EDGE] =
    g_param_spec_enum ("edge",
                       "Edge",
                       "The edge for the tab-strip",
                       GTK_TYPE_POSITION_TYPE,
                       GTK_POS_TOP,
                       (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_STYLE] =
    g_param_spec_flags ("style",
                        "Style",
                        "The tab style",
                        DZL_TYPE_TAB_STYLE,
                        DZL_TAB_BOTH,
                        (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_STACK] =
    g_param_spec_object ("stack",
                         "Stack",
                         "The stack of items to manage.",
                         GTK_TYPE_STACK,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_css_name (widget_class, "dzltabstrip");
}

static void
dzl_tab_strip_init (DzlTabStrip *self)
{
  DzlTabStripPrivate *priv = dzl_tab_strip_get_instance_private (self);
  GSimpleActionGroup *group;
  static const GActionEntry entries[] = {
    { "tab", NULL, "i", "0", set_tab_state },
  };

  priv->style = DZL_TAB_BOTH;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);

  group = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (group), entries, G_N_ELEMENTS (entries), self);
  priv->action = g_object_ref (g_action_map_lookup_action (G_ACTION_MAP (group), "tab"));
  gtk_widget_insert_action_group (GTK_WIDGET (self), "tab-strip", G_ACTION_GROUP (group));
  g_object_unref (group);

  dzl_tab_strip_set_edge (self, GTK_POS_TOP);
}

static void
dzl_tab_strip_child_position_changed (DzlTabStrip *self,
                                      GParamSpec  *pspec,
                                      GtkWidget   *child)
{
  GVariant *state;
  GtkWidget *parent;
  DzlTab *tab;
  gint position = -1;

  g_warn_if_fail (DZL_IS_TAB_STRIP (self));
  g_warn_if_fail (GTK_IS_WIDGET (child));

  tab = g_object_get_data (G_OBJECT (child), "DZL_TAB");

  if (!tab || !DZL_IS_TAB (tab))
    {
      g_warning ("Child %s (%p) is missing backpointer to tab",
                 G_OBJECT_TYPE_NAME (child), child);
      return;
    }

  parent = gtk_widget_get_parent (child);

  g_warn_if_fail (GTK_IS_STACK (parent));

  gtk_container_child_get (GTK_CONTAINER (parent), child,
                           "position", &position,
                           NULL);

  if (position < 0)
    {
      g_warning ("Improbable position for child, %d", position);
      return;
    }

  gtk_container_child_set (GTK_CONTAINER (self), GTK_WIDGET (tab),
                           "position", position,
                           NULL);

  state = g_variant_new_int32 (position);
  gtk_actionable_set_action_target_value (GTK_ACTIONABLE (tab), state);

  dzl_tab_strip_update_action_targets (self);
}

static void
dzl_tab_strip_child_title_changed (DzlTabStrip *self,
                                   GParamSpec  *pspec,
                                   GtkWidget   *child)
{
  gchar *title = NULL;
  GtkWidget *parent;
  DzlTab *tab;

  g_warn_if_fail (DZL_IS_TAB_STRIP (self));
  g_warn_if_fail (GTK_IS_WIDGET (child));

  tab = g_object_get_data (G_OBJECT (child), "DZL_TAB");

  if (!DZL_IS_TAB (tab))
    return;

  parent = gtk_widget_get_parent (child);

  g_warn_if_fail (GTK_IS_STACK (parent));

  gtk_container_child_get (GTK_CONTAINER (parent), child,
                           "title", &title,
                           NULL);

  dzl_tab_set_title (tab, title);

  g_free (title);
}

static void
dzl_tab_strip_child_icon_name_changed (DzlTabStrip *self,
                                       GParamSpec  *pspec,
                                       GtkWidget   *child)
{
  gchar *icon_name = NULL;
  GtkWidget *parent;
  DzlTab *tab;

  g_warn_if_fail (DZL_IS_TAB_STRIP (self));
  g_warn_if_fail (GTK_IS_WIDGET (child));

  tab = g_object_get_data (G_OBJECT (child), "DZL_TAB");

  if (!DZL_IS_TAB (tab))
    return;

  parent = gtk_widget_get_parent (child);

  g_warn_if_fail (GTK_IS_STACK (parent));

  gtk_container_child_get (GTK_CONTAINER (parent), child,
                           "icon-name", &icon_name,
                           NULL);

  dzl_tab_set_icon_name (tab, icon_name);

  g_free (icon_name);
}

static void
dzl_tab_strip_stack_notify_visible_child (DzlTabStrip *self,
                                          GParamSpec  *pspec,
                                          GtkStack    *stack)
{
  GtkWidget *visible;

  g_warn_if_fail (DZL_IS_TAB_STRIP (self));
  g_warn_if_fail (GTK_IS_STACK (stack));

  visible = gtk_stack_get_visible_child (stack);

  if (visible != NULL)
    {
      DzlTab *tab = g_object_get_data (G_OBJECT (visible), "DZL_TAB");

      if (DZL_IS_TAB (tab))
        dzl_tab_set_active (DZL_TAB (tab), TRUE);
    }
}

static void
dzl_tab_strip_tab_clicked (DzlTabStrip *self,
                           DzlTab      *tab)
{
  GtkWidget *widget;

  g_warn_if_fail (DZL_IS_TAB_STRIP (self));
  g_warn_if_fail (DZL_IS_TAB (tab));

  if (NULL != (widget = dzl_tab_get_widget (tab)))
    {
      if (dzl_tab_get_active (tab))
        gtk_widget_grab_focus (widget);
    }
}

static void
dzl_tab_strip_stack_add (DzlTabStrip *self,
                         GtkWidget   *widget,
                         GtkStack    *stack)
{
  DzlTabStripPrivate *priv = dzl_tab_strip_get_instance_private (self);
  g_autoptr(GVariant) target = g_variant_ref_sink (g_variant_new_int32 (0));
  DzlTab *tab;
  gboolean can_close = FALSE;

  g_warn_if_fail (DZL_IS_TAB_STRIP (self));
  g_warn_if_fail (GTK_IS_WIDGET (widget));
  g_warn_if_fail (GTK_IS_STACK (stack));

  if (DZL_IS_DOCK_ITEM (widget))
    can_close = dzl_dock_item_get_can_close (DZL_DOCK_ITEM (widget));

  tab = g_object_new (DZL_TYPE_TAB,
                      "action-name", "tab-strip.tab",
                      "action-target", target,
                      "can-close", can_close,
                      "edge", priv->edge,
                      "style", priv->style,
                      "widget", widget,
                      NULL);

  g_object_set_data (G_OBJECT (widget), "DZL_TAB", tab);

  g_signal_connect_object (tab,
                           "clicked",
                           G_CALLBACK (dzl_tab_strip_tab_clicked),
                           self,
                           G_CONNECT_SWAPPED | G_CONNECT_AFTER);

  g_signal_connect_object (widget,
                           "child-notify::position",
                           G_CALLBACK (dzl_tab_strip_child_position_changed),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (widget,
                           "child-notify::title",
                           G_CALLBACK (dzl_tab_strip_child_title_changed),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (widget,
                           "child-notify::icon-name",
                           G_CALLBACK (dzl_tab_strip_child_icon_name_changed),
                           self,
                           G_CONNECT_SWAPPED);

  gtk_container_add_with_properties (GTK_CONTAINER (self), GTK_WIDGET (tab),
                                     "pack-type", GTK_PACK_START,
                                     "expand", TRUE,
                                     "fill", TRUE,
                                     NULL);

  g_object_bind_property (widget, "visible", tab, "visible", G_BINDING_SYNC_CREATE);

  if (DZL_IS_DOCK_WIDGET (widget))
    g_object_bind_property (widget, "can-close", tab, "can-close", 0);

  dzl_tab_strip_child_title_changed (self, NULL, widget);
  dzl_tab_strip_child_icon_name_changed (self, NULL, widget);
  dzl_tab_strip_stack_notify_visible_child (self, NULL, stack);
}

static void
dzl_tab_strip_stack_remove (DzlTabStrip *self,
                            GtkWidget   *widget,
                            GtkStack    *stack)
{
  DzlTab *tab;

  g_warn_if_fail (DZL_IS_TAB_STRIP (self));
  g_warn_if_fail (GTK_IS_WIDGET (widget));
  g_warn_if_fail (GTK_IS_STACK (stack));

  tab = g_object_get_data (G_OBJECT (widget), "DZL_TAB");

  if (DZL_IS_TAB (tab))
    {
      g_object_set_data (G_OBJECT (widget), "DZL_TAB", NULL);
      gtk_container_remove (GTK_CONTAINER (self), GTK_WIDGET (tab));
    }
}

GtkWidget *
dzl_tab_strip_new (void)
{
  return g_object_new (DZL_TYPE_TAB_STRIP, NULL);
}

/**
 * dzl_tab_strip_get_stack:
 *
 * Returns: (transfer none) (nullable): A #GtkStack or %NULL.
 */
GtkStack *
dzl_tab_strip_get_stack (DzlTabStrip *self)
{
  DzlTabStripPrivate *priv = dzl_tab_strip_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_TAB_STRIP (self), NULL);

  return priv->stack;
}

static void
dzl_tab_strip_cold_plug (GtkWidget *widget,
                         gpointer   user_data)
{
  DzlTabStrip *self = user_data;
  DzlTabStripPrivate *priv = dzl_tab_strip_get_instance_private (self);

  g_warn_if_fail (DZL_IS_TAB_STRIP (self));
  g_warn_if_fail (GTK_IS_WIDGET (widget));

  dzl_tab_strip_stack_add (self, widget, priv->stack);
}

void
dzl_tab_strip_set_stack (DzlTabStrip *self,
                         GtkStack    *stack)
{
  DzlTabStripPrivate *priv = dzl_tab_strip_get_instance_private (self);

  g_return_if_fail (DZL_IS_TAB_STRIP (self));
  g_return_if_fail (!stack || GTK_IS_STACK (stack));

  if (stack != priv->stack)
    {
      if (priv->stack != NULL)
        {
          g_signal_handlers_disconnect_by_func (priv->stack,
                                                G_CALLBACK (dzl_tab_strip_stack_notify_visible_child),
                                                self);

          g_signal_handlers_disconnect_by_func (priv->stack,
                                                G_CALLBACK (dzl_tab_strip_stack_add),
                                                self);

          g_signal_handlers_disconnect_by_func (priv->stack,
                                                G_CALLBACK (dzl_tab_strip_stack_remove),
                                                self);

          z_gtk_container_destroy_all_children (
            GTK_CONTAINER (self));
          /*gtk_container_foreach (GTK_CONTAINER (self), (GtkCallback)gtk_widget_destroy, NULL);*/

          g_clear_object (&priv->stack);
        }

      if (stack != NULL)
        {
          priv->stack = g_object_ref (stack);

          g_signal_connect_object (priv->stack,
                                   "notify::visible-child",
                                   G_CALLBACK (dzl_tab_strip_stack_notify_visible_child),
                                   self,
                                   G_CONNECT_SWAPPED);

          g_signal_connect_object (priv->stack,
                                   "add",
                                   G_CALLBACK (dzl_tab_strip_stack_add),
                                   self,
                                   G_CONNECT_SWAPPED);

          g_signal_connect_object (priv->stack,
                                   "remove",
                                   G_CALLBACK (dzl_tab_strip_stack_remove),
                                   self,
                                   G_CONNECT_SWAPPED);

          gtk_container_foreach (GTK_CONTAINER (priv->stack),
                                 dzl_tab_strip_cold_plug,
                                 self);
        }
    }
}

static void
dzl_tab_strip_update_edge (GtkWidget *widget,
                           gpointer   user_data)
{
  GtkPositionType edge = GPOINTER_TO_INT (user_data);

  g_warn_if_fail (GTK_IS_WIDGET (widget));

  if (DZL_IS_TAB (widget))
    dzl_tab_set_edge (DZL_TAB (widget), edge);
}

GtkPositionType
dzl_tab_strip_get_edge (DzlTabStrip *self)
{
  DzlTabStripPrivate *priv = dzl_tab_strip_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_TAB_STRIP (self), 0);

  return priv->edge;
}

void
dzl_tab_strip_set_edge (DzlTabStrip     *self,
                        GtkPositionType  edge)
{
  DzlTabStripPrivate *priv = dzl_tab_strip_get_instance_private (self);

  g_return_if_fail (DZL_IS_TAB_STRIP (self));
  g_return_if_fail (edge >= 0);
  g_return_if_fail (edge <= 3);

  if (priv->edge != edge)
    {
      GtkStyleContext *style_context;
      const gchar *class_name = NULL;

      priv->edge = edge;

      gtk_container_foreach (GTK_CONTAINER (self),
                             dzl_tab_strip_update_edge,
                             GINT_TO_POINTER (edge));

      style_context = gtk_widget_get_style_context (GTK_WIDGET (self));

      gtk_style_context_remove_class (style_context, "left");
      gtk_style_context_remove_class (style_context, "top");
      gtk_style_context_remove_class (style_context, "right");
      gtk_style_context_remove_class (style_context, "bottom");

      switch (edge)
        {
        case GTK_POS_LEFT:
          class_name = "left";
          break;

        case GTK_POS_RIGHT:
          class_name = "right";
          break;

        case GTK_POS_TOP:
          class_name = "top";
          break;

        case GTK_POS_BOTTOM:
          class_name = "bottom";
          break;

        default:
          g_warn_if_reached ();
        }

      gtk_style_context_add_class (style_context, class_name);

      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_EDGE]);
    }
}

static void
apply_style (GtkWidget *widget,
             gpointer   user_data)
{
  DzlTabStyle style = GPOINTER_TO_UINT (user_data);

  g_warn_if_fail (GTK_IS_WIDGET (widget));

  if (DZL_IS_TAB (widget))
    dzl_tab_set_style (DZL_TAB (widget), style);
}

guint
dzl_tab_strip_get_style (DzlTabStrip *self)
{
  DzlTabStripPrivate *priv = dzl_tab_strip_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_TAB_STRIP (self), 0);

  return priv->style;
}

void
dzl_tab_strip_set_style (DzlTabStrip *self,
                         DzlTabStyle  style)
{
  DzlTabStripPrivate *priv = dzl_tab_strip_get_instance_private (self);

  g_return_if_fail (DZL_IS_TAB_STRIP (self));

  if (style != priv->style)
    {
      priv->style = style;
      gtk_container_foreach (GTK_CONTAINER (self), apply_style, GUINT_TO_POINTER (style));
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_STYLE]);
    }
}

void
dzl_tab_strip_add_control (DzlTabStrip *self,
                           GtkWidget   *widget)
{
  g_return_if_fail (DZL_IS_TAB_STRIP (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_container_add_with_properties (GTK_CONTAINER (self), widget,
                                     "pack-type", GTK_PACK_END,
                                     "expand", FALSE,
                                     "fill", FALSE,
                                     NULL);

  gtk_style_context_add_class (gtk_widget_get_style_context (widget), "control");
}

static void
dzl_tab_strip_add_child (GtkBuildable *buildable,
                         GtkBuilder   *builder,
                         GObject      *child,
                         const gchar  *child_type)
{
  DzlTabStrip *self = (DzlTabStrip *)buildable;

  g_warn_if_fail (DZL_IS_TAB_STRIP (self));
  g_warn_if_fail (GTK_IS_BUILDER (builder));
  g_warn_if_fail (G_IS_OBJECT (child));

  if (g_strcmp0 (child_type, "control") == 0 && GTK_IS_WIDGET (child))
    dzl_tab_strip_add_control (self, GTK_WIDGET (child));
  else
    g_warning ("I do not know how to add %s of type %s",
               G_OBJECT_TYPE_NAME (child), child_type ? child_type : "NULL");
}

static void
buildable_iface_init (GtkBuildableIface *iface)
{
  iface->add_child = dzl_tab_strip_add_child;
}
