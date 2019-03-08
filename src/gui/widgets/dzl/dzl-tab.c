/* dzl-tab.c
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
#include "gui/widgets/dzl/dzl-tab.h"
#include "gui/widgets/dzl/dzl-tab-private.h"
#include "gui/widgets/dzl/dzl-util-private.h"

struct _DzlTab { GtkEventBox parent; };

typedef struct
{
  /*
   * The edge the tab is being displayed upon. We use this to rotate
   * text or alter box orientation.
   */
  GtkPositionType edge : 2;

  /* Can we be closed by the user */
  guint can_close : 1;

  /* Are we the active tab */
  guint active : 1;

  /* If we are currently pressed */
  guint pressed : 1;

  /* If the pointer is over the widget (prelight) */
  guint pointer_in_widget : 1;

  /* If we are currently activating */
  guint in_activate : 1;

  /* Our icon/text style */
  DzlTabStyle style : 2;

  /* The action name to change the current tab */
  gchar *action_name;

  /* The value for the current tab */
  GVariant *action_target_value;

  /*
   * Because we don't have access to GtkActionMuxer or GtkActionHelper
   * from inside of Gtk, we need to manually track the state of the
   * action we are monitoring.
   *
   * We hold a pointer to the group so we can disconnect as necessary.
   */
  GActionGroup *action_group;
  gulong action_state_changed_handler;

  /*
   * These are our control widgets. The box contains the title in the
   * center with the minimize/close buttons to a side, depending on the
   * orientation/edge of the tabs.
   */
  GtkBox *box;
  GtkImage *image;
  GtkLabel *title;
  GtkWidget *close;
  GtkWidget *minimize;

  /*
   * This is a weak pointer to the widget this tab represents.
   * It's used by the stack to translate between the tab/widget
   * as necessary.
   */
  GtkWidget *widget;
} DzlTabPrivate;

static void actionable_iface_init (GtkActionableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (DzlTab, dzl_tab, DZL_TYPE_BIN,
                         G_ADD_PRIVATE (DzlTab)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ACTIONABLE, actionable_iface_init))

enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_CAN_CLOSE,
  PROP_EDGE,
  PROP_STYLE,
  PROP_TITLE,
  PROP_WIDGET,
  N_PROPS,

  PROP_ACTION_NAME,
  PROP_ACTION_TARGET,
};

enum {
  CLICKED,
  N_SIGNALS
};

static GParamSpec *properties [N_PROPS];
static guint signals [N_SIGNALS];

static void
dzl_tab_get_inner_allocation (DzlTab        *self,
                              GtkAllocation *alloc)
{
  GtkBorder margin;
  GtkBorder border;
  GtkStyleContext *style_context;
  GtkStateFlags flags;

  g_assert (DZL_IS_TAB (self));
  g_assert (alloc != NULL);

  gtk_widget_get_allocation (GTK_WIDGET (self), alloc);

  style_context = gtk_widget_get_style_context (GTK_WIDGET (self));
  flags = gtk_widget_get_state_flags (GTK_WIDGET (self));

  gtk_style_context_get_border (style_context, flags, &margin);
  gtk_style_context_get_border (style_context, flags, &border);

  alloc->x += border.left;
  alloc->x += margin.left;

  alloc->y += border.top;
  alloc->y += margin.top;

  alloc->width -= (border.right + border.left);
  alloc->width -= (margin.right + margin.left);

  alloc->height -= (border.bottom + border.top);
  alloc->height -= (margin.bottom + margin.top);
}

static void
dzl_tab_apply_state (DzlTab *self)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_assert (DZL_IS_TAB (self));

  if (priv->active)
    gtk_widget_set_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_CHECKED, FALSE);
  else
    gtk_widget_unset_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_CHECKED);
}

static void
dzl_tab_activate (DzlTab *self)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);
  g_autoptr(GVariant) value = NULL;

  g_assert (DZL_IS_TAB (self));

  if (priv->in_activate ||
      priv->action_name == NULL ||
      priv->action_target_value == NULL)
    return;

  priv->in_activate = TRUE;

  value = dzl_gtk_widget_get_action_state (GTK_WIDGET (self), priv->action_name);
  if (value != NULL && !g_variant_equal (value, priv->action_target_value))
    dzl_gtk_widget_activate_action (GTK_WIDGET (self), priv->action_name, priv->action_target_value);

  priv->in_activate = FALSE;
}

static void
dzl_tab_destroy (GtkWidget *widget)
{
  DzlTab *self = (DzlTab *)widget;
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  if (priv->action_group != NULL)
    {
      g_signal_handler_disconnect (priv->action_group,
                                   priv->action_state_changed_handler);
      priv->action_state_changed_handler = 0;
      dzl_clear_weak_pointer (&priv->action_group);
    }

  dzl_clear_weak_pointer (&priv->widget);
  g_clear_pointer (&priv->action_name, g_free);
  g_clear_pointer (&priv->action_target_value, g_variant_unref);

  GTK_WIDGET_CLASS (dzl_tab_parent_class)->destroy (widget);
}

static void
dzl_tab_update_edge (DzlTab *self)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_assert (DZL_IS_TAB (self));

  switch (priv->edge)
    {
    case GTK_POS_TOP:
      gtk_label_set_angle (priv->title, 0.0);
      gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->box), GTK_ORIENTATION_HORIZONTAL);
      gtk_box_set_child_packing (priv->box, GTK_WIDGET (priv->close), FALSE, FALSE, 0, GTK_PACK_END);
      gtk_box_set_child_packing (priv->box, GTK_WIDGET (priv->minimize), FALSE, FALSE, 0, GTK_PACK_END);
      gtk_widget_set_hexpand (GTK_WIDGET (self), TRUE);
      gtk_widget_set_vexpand (GTK_WIDGET (self), FALSE);
      break;

    case GTK_POS_BOTTOM:
      gtk_label_set_angle (priv->title, 0.0);
      gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->box), GTK_ORIENTATION_HORIZONTAL);
      gtk_box_set_child_packing (priv->box, GTK_WIDGET (priv->close), FALSE, FALSE, 0, GTK_PACK_END);
      gtk_box_set_child_packing (priv->box, GTK_WIDGET (priv->minimize), FALSE, FALSE, 0, GTK_PACK_END);
      gtk_widget_set_hexpand (GTK_WIDGET (self), TRUE);
      gtk_widget_set_vexpand (GTK_WIDGET (self), FALSE);
      break;

    case GTK_POS_LEFT:
      gtk_label_set_angle (priv->title, -90.0);
      gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->box), GTK_ORIENTATION_VERTICAL);
      gtk_box_set_child_packing (priv->box, GTK_WIDGET (priv->close), FALSE, FALSE, 0, GTK_PACK_END);
      gtk_box_set_child_packing (priv->box, GTK_WIDGET (priv->minimize), FALSE, FALSE, 0, GTK_PACK_END);
      gtk_widget_set_hexpand (GTK_WIDGET (self), FALSE);
      gtk_widget_set_vexpand (GTK_WIDGET (self), TRUE);
      break;

    case GTK_POS_RIGHT:
      gtk_label_set_angle (priv->title, 90.0);
      gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->box), GTK_ORIENTATION_VERTICAL);
      gtk_box_set_child_packing (priv->box, GTK_WIDGET (priv->close), FALSE, FALSE, 0, GTK_PACK_START);
      gtk_box_set_child_packing (priv->box, GTK_WIDGET (priv->minimize), FALSE, FALSE, 0, GTK_PACK_START);
      gtk_widget_set_hexpand (GTK_WIDGET (self), FALSE);
      gtk_widget_set_vexpand (GTK_WIDGET (self), TRUE);
      break;

    default:
      g_assert_not_reached ();
    }
}

static gboolean
get_widget_coordinates (GtkWidget *widget,
                        GdkEvent  *event,
                        gdouble   *x,
                        gdouble   *y)
{
  GdkWindow *window = ((GdkEventAny *)event)->window;
  GdkWindow *target = gtk_widget_get_window (widget);
  gdouble tx, ty;

  if (!gdk_event_get_coords (event, &tx, &ty))
    return FALSE;

  while (window && window != target)
    {
      gint window_x, window_y;

      gdk_window_get_position (window, &window_x, &window_y);
      tx += window_x;
      ty += window_y;

      window = gdk_window_get_parent (window);
    }

  if (window)
    {
      *x = tx;
      *y = ty;

      return TRUE;
    }
  else
    return FALSE;
}

static void
dzl_tab_update_prelight (DzlTab   *self,
                         GdkEvent *event)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);
  gdouble x, y;

  g_assert (DZL_IS_TAB (self));
  g_assert (event != NULL);

  if (get_widget_coordinates (GTK_WIDGET (self), (GdkEvent *)event, &x, &y))
    {
      GtkAllocation alloc;

      dzl_tab_get_inner_allocation (self, &alloc);

      /*
       * We've translated our x,y coords to be relative to the widget, so we
       * can discard the x,y of the allocation.
       */
      alloc.x = 0;
      alloc.y = 0;

      if (x >= alloc.x &&
          x <= (alloc.x + alloc.width) &&
          y >= alloc.y &&
          y <= (alloc.y + alloc.height))
        {
          priv->pointer_in_widget = TRUE;
          gtk_widget_set_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_PRELIGHT, FALSE);
          return;
        }
    }

  priv->pointer_in_widget = FALSE;
  gtk_widget_unset_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_PRELIGHT);
}

static gboolean
dzl_tab_motion_notify_event (GtkWidget      *widget,
                             GdkEventMotion *event)
{
  DzlTab *self = (DzlTab *)widget;

  g_assert (DZL_IS_TAB (self));
  g_assert (event != NULL);

  dzl_tab_update_prelight (self, (GdkEvent *)event);

  return GDK_EVENT_PROPAGATE;
}

static gboolean
dzl_tab_enter_notify_event (GtkWidget        *widget,
                            GdkEventCrossing *event)
{
  DzlTab *self = (DzlTab *)widget;

  g_assert (DZL_IS_TAB (self));
  g_assert (event != NULL);

  dzl_tab_update_prelight (self, (GdkEvent *)event);

  return GDK_EVENT_PROPAGATE;
}

static gboolean
dzl_tab_leave_notify_event (GtkWidget        *widget,
                            GdkEventCrossing *event)
{
  DzlTab *self = (DzlTab *)widget;

  g_return_val_if_fail (DZL_IS_TAB (self), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  dzl_tab_update_prelight (self, (GdkEvent *)event);

  return GDK_EVENT_PROPAGATE;
}

static gboolean
dzl_tab_button_press_event (GtkWidget      *widget,
                            GdkEventButton *event)
{
  DzlTab *self = (DzlTab *)widget;
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_TAB (self), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->button == GDK_BUTTON_PRIMARY)
    {
      priv->pressed = TRUE;
      gtk_widget_set_state_flags (widget, GTK_STATE_FLAG_ACTIVE, FALSE);
      gtk_widget_grab_focus (GTK_WIDGET (self));
      return GDK_EVENT_STOP;
    }

  return GDK_EVENT_PROPAGATE;
}

static gboolean
dzl_tab_button_release_event (GtkWidget      *widget,
                              GdkEventButton *event)
{
  DzlTab *self = (DzlTab *)widget;
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_TAB (self), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->button == GDK_BUTTON_PRIMARY)
    {
      priv->pressed = FALSE;
      gtk_widget_unset_state_flags (widget, GTK_STATE_FLAG_ACTIVE);

      if (priv->pointer_in_widget)
        g_signal_emit (self, signals [CLICKED], 0);

      return GDK_EVENT_STOP;
    }

  return GDK_EVENT_PROPAGATE;
}

static void
dzl_tab_realize (GtkWidget *widget)
{
  DzlTab *self = (DzlTab *)widget;
  GdkWindowAttr attributes = { 0 };
  GdkWindow *parent;
  GdkWindow *window;
  GtkAllocation alloc;
  gint attributes_mask = 0;

  g_assert (DZL_IS_TAB (widget));

  gtk_widget_get_allocation (GTK_WIDGET (self), &alloc);

  gtk_widget_set_realized (GTK_WIDGET (self), TRUE);

  parent = gtk_widget_get_parent_window (GTK_WIDGET (self));

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.x = alloc.x;
  attributes.y = alloc.y;
  attributes.width = alloc.width;
  attributes.height = alloc.height;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
                            GDK_BUTTON_RELEASE_MASK |
                            GDK_TOUCH_MASK |
                            GDK_EXPOSURE_MASK |
                            GDK_ENTER_NOTIFY_MASK |
                            GDK_LEAVE_NOTIFY_MASK);

  attributes_mask = (GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL);

  window = gdk_window_new (parent, &attributes, attributes_mask);
  gtk_widget_register_window (GTK_WIDGET (self), window);
  gtk_widget_set_window (GTK_WIDGET (self), window);
}

static void
dzl_tab_action_state_changed (DzlTab       *self,
                              const gchar  *action_name,
                              GVariant     *value,
                              GActionGroup *group)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);
  gboolean active;

  g_assert (DZL_IS_TAB (self));
  g_assert (action_name != NULL);
  g_assert (G_IS_ACTION_GROUP (group));

  active = (value != NULL &&
            priv->action_target_value != NULL &&
            g_variant_equal (value, priv->action_target_value));

  if (active != priv->active)
    {
      priv->active = active;
      dzl_tab_apply_state (self);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIVE]);
    }
}

static void
dzl_tab_monitor_action_group (DzlTab       *self,
                              GActionGroup *group)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_if_fail (DZL_IS_TAB (self));
  g_return_if_fail (!group || G_IS_ACTION_GROUP (group));

  if (group != priv->action_group)
    {
      if (priv->action_group != NULL)
        {
          g_signal_handler_disconnect (priv->action_group,
                                       priv->action_state_changed_handler);
          priv->action_state_changed_handler = 0;
          dzl_clear_weak_pointer (&priv->action_group);
        }

      if (group != NULL)
        {
          gchar *prefix = NULL;
          gchar *name = NULL;

          dzl_g_action_name_parse (priv->action_name, &prefix, &name);

          if (name != NULL)
            {
              gchar *detailed;

              detailed = g_strdup_printf ("action-state-changed::%s", name);
              dzl_set_weak_pointer (&priv->action_group, group);
              priv->action_state_changed_handler =
                g_signal_connect_object (priv->action_group,
                                         detailed,
                                         G_CALLBACK (dzl_tab_action_state_changed),
                                         self,
                                         G_CONNECT_SWAPPED);

              g_free (detailed);
            }

          g_free (prefix);
          g_free (name);
        }
    }
}

static void
dzl_tab_hierarchy_changed (GtkWidget *widget,
                           GtkWidget *old_toplevel)
{
  DzlTab *self = (DzlTab *)widget;
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);
  GActionGroup *group;

  g_assert (GTK_IS_WIDGET (widget));
  g_assert (!old_toplevel || GTK_IS_WIDGET (old_toplevel));

  group = dzl_gtk_widget_find_group_for_action (widget, priv->action_name);
  dzl_tab_monitor_action_group (self, group);
}

static void
dzl_tab_size_allocate (GtkWidget     *widget,
                       GtkAllocation *allocation)
{
  g_assert (DZL_IS_TAB (widget));

  GTK_WIDGET_CLASS (dzl_tab_parent_class)->size_allocate (widget, allocation);

  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (gtk_widget_get_window (widget),
                            allocation->x,
                            allocation->y,
                            allocation->width,
                            allocation->height);
}

static void
dzl_tab_close_clicked (DzlTab    *self,
                       GtkWidget *button)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_assert (DZL_IS_TAB (self));
  g_assert (GTK_IS_BUTTON (button));

  g_object_ref (self);

  if (DZL_IS_DOCK_ITEM (priv->widget) &&
      dzl_dock_item_get_can_close (DZL_DOCK_ITEM (priv->widget)))
    dzl_dock_item_close (DZL_DOCK_ITEM (priv->widget));

  g_object_unref (self);
}

static void
dzl_tab_minimize_clicked (DzlTab    *self,
                          GtkWidget *button)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);
  GtkPositionType position = GTK_POS_LEFT;

  g_assert (DZL_IS_TAB (self));
  g_assert (GTK_IS_BUTTON (button));

  g_object_ref (self);

  if (DZL_IS_DOCK_ITEM (priv->widget))
    {
      DzlDockItem *item = DZL_DOCK_ITEM (priv->widget);

      for (DzlDockItem *parent = dzl_dock_item_get_parent (item);
           parent != NULL;
           parent = dzl_dock_item_get_parent (parent))
        {
          if (dzl_dock_item_minimize (parent, item, &position))
            break;
        }
    }

  g_object_unref (self);
}

static gboolean
dzl_tab_query_tooltip (GtkWidget  *widget,
                       gint        x,
                       gint        y,
                       gboolean    keyboard,
                       GtkTooltip *tooltip)
{
  DzlTab *self = (DzlTab *)widget;
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);
  const gchar *title;

  g_assert (DZL_IS_TAB (self));
  g_assert (GTK_IS_TOOLTIP (tooltip));

  title = gtk_label_get_label (priv->title);

  if (title != NULL)
    {
      gtk_tooltip_set_text (tooltip, title);
      return TRUE;
    }

  return FALSE;
}

static void
dzl_tab_get_property (GObject    *object,
                      guint       prop_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  DzlTab *self = DZL_TAB (object);
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, dzl_tab_get_active (self));
      break;

    case PROP_ACTION_NAME:
      g_value_set_string (value, priv->action_name);
      break;

    case PROP_ACTION_TARGET:
      g_value_set_boxed (value, priv->action_target_value);
      break;

    case PROP_CAN_CLOSE:
      g_value_set_boolean (value, dzl_tab_get_can_close (self));
      break;

    case PROP_EDGE:
      g_value_set_enum (value, dzl_tab_get_edge (self));
      break;

    case PROP_STYLE:
      g_value_set_flags (value, dzl_tab_get_style (self));
      break;

    case PROP_TITLE:
      g_value_set_string (value, dzl_tab_get_title (self));
      break;

    case PROP_WIDGET:
      g_value_set_object (value, dzl_tab_get_widget (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_tab_set_property (GObject      *object,
                      guint         prop_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  DzlTab *self = DZL_TAB (object);
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      dzl_tab_set_active (self, g_value_get_boolean (value));
      break;

    case PROP_ACTION_NAME:
      g_free (priv->action_name);
      priv->action_name = g_value_dup_string (value);
      break;

    case PROP_ACTION_TARGET:
      g_clear_pointer (&priv->action_target_value, g_variant_unref);
      priv->action_target_value = g_value_get_variant (value);
      if (priv->action_target_value != NULL)
        g_variant_ref_sink (priv->action_target_value);
      break;

    case PROP_CAN_CLOSE:
      dzl_tab_set_can_close (self, g_value_get_boolean (value));
      break;

    case PROP_EDGE:
      dzl_tab_set_edge (self, g_value_get_enum (value));
      break;

    case PROP_STYLE:
      dzl_tab_set_style (self, g_value_get_flags (value));
      break;

    case PROP_TITLE:
      dzl_tab_set_title (self, g_value_get_string (value));
      break;

    case PROP_WIDGET:
      dzl_tab_set_widget (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
dzl_tab_class_init (DzlTabClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = dzl_tab_get_property;
  object_class->set_property = dzl_tab_set_property;

  widget_class->destroy = dzl_tab_destroy;
  widget_class->button_press_event = dzl_tab_button_press_event;
  widget_class->button_release_event = dzl_tab_button_release_event;
  widget_class->motion_notify_event = dzl_tab_motion_notify_event;
  widget_class->enter_notify_event = dzl_tab_enter_notify_event;
  widget_class->leave_notify_event = dzl_tab_leave_notify_event;
  widget_class->realize = dzl_tab_realize;
  widget_class->size_allocate = dzl_tab_size_allocate;
  widget_class->hierarchy_changed = dzl_tab_hierarchy_changed;
  widget_class->query_tooltip = dzl_tab_query_tooltip;

  gtk_widget_class_set_css_name (widget_class, "dzltab");

  g_object_class_override_property (object_class, PROP_ACTION_NAME, "action-name");
  g_object_class_override_property (object_class, PROP_ACTION_TARGET, "action-target");

  properties [PROP_ACTIVE] =
    g_param_spec_boolean ("active",
                          "Active",
                          "If the tab is currently active",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_CAN_CLOSE] =
    g_param_spec_boolean ("can-close",
                          "Can Close",
                          "If the tab widget can be closed",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_EDGE] =
    g_param_spec_enum ("edge",
                       "Edge",
                       "Edge",
                       GTK_TYPE_POSITION_TYPE,
                       GTK_POS_TOP,
                       (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_STYLE] =
    g_param_spec_flags ("style",
                        "Style",
                        "The style for the tab",
                        DZL_TYPE_TAB_STYLE,
                        DZL_TAB_BOTH,
                        (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "Title",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_WIDGET] =
    g_param_spec_object ("widget",
                         "Widget",
                         "The widget the tab represents",
                         GTK_TYPE_WIDGET,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signals [CLICKED] =
    g_signal_new_class_handler ("clicked",
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_LAST,
                                G_CALLBACK (dzl_tab_activate),
                                NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void
dzl_tab_init (DzlTab *self)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);
  GtkBox *center;

  priv->style = DZL_TAB_BOTH;
  priv->edge = GTK_POS_TOP;

  gtk_widget_set_has_window (GTK_WIDGET (self), TRUE);

  gtk_widget_add_events (GTK_WIDGET (self),
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_ENTER_NOTIFY_MASK|
                         GDK_LEAVE_NOTIFY_MASK);
  gtk_widget_set_hexpand (GTK_WIDGET (self), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (self), FALSE);

  gtk_widget_set_has_tooltip (GTK_WIDGET (self), TRUE);

  priv->box = g_object_new (GTK_TYPE_BOX,
                            "orientation", GTK_ORIENTATION_HORIZONTAL,
                            "visible", TRUE,
                            NULL);
  g_signal_connect (priv->box, "destroy", G_CALLBACK (gtk_widget_destroyed), &priv->box);
  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (priv->box));

  center = g_object_new (GTK_TYPE_BOX,
                         "spacing", 6,
                         "visible", TRUE,
                         NULL);
  gtk_box_set_center_widget (priv->box, GTK_WIDGET (center));

  priv->image = g_object_new (GTK_TYPE_IMAGE,
                              "visible", TRUE,
                              NULL);
  g_signal_connect (priv->image, "destroy", G_CALLBACK (gtk_widget_destroyed), &priv->image);
  gtk_box_pack_start (center, GTK_WIDGET (priv->image), FALSE, FALSE, 0);

  priv->title = g_object_new (GTK_TYPE_LABEL,
                              "ellipsize", PANGO_ELLIPSIZE_END,
                              "use-underline", TRUE,
                              "visible", TRUE,
                              NULL);
  g_signal_connect (priv->title, "destroy", G_CALLBACK (gtk_widget_destroyed), &priv->title);
  gtk_box_pack_start (center, GTK_WIDGET (priv->title), FALSE, FALSE, 0);

  priv->close = g_object_new (GTK_TYPE_BUTTON,
                              "halign", GTK_ALIGN_END,
                              "child", g_object_new (GTK_TYPE_IMAGE,
                                                     "icon-name", "window-close-symbolic",
                                                     "visible", TRUE,
                                                     NULL),
                              "visible", TRUE,
                              NULL);
  g_signal_connect_object (priv->close,
                           "clicked",
                           G_CALLBACK (dzl_tab_close_clicked),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (priv->close)), "close");
  gtk_box_pack_end (priv->box, GTK_WIDGET (priv->close), FALSE, FALSE, 0);
  g_object_bind_property (self, "can-close", priv->close, "visible", G_BINDING_SYNC_CREATE);

  priv->minimize = g_object_new (GTK_TYPE_BUTTON,
                                 "halign", GTK_ALIGN_END,
                                 "child", g_object_new (GTK_TYPE_IMAGE,
                                                        "icon-name", "window-minimize-symbolic",
                                                        "visible", TRUE,
                                                        NULL),
                                 "visible", TRUE,
                                 NULL);
  g_signal_connect_object (priv->minimize,
                           "clicked",
                           G_CALLBACK (dzl_tab_minimize_clicked),
                           self,
                           G_CONNECT_SWAPPED);
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (priv->minimize)), "minimize");
  gtk_box_pack_end (priv->box, GTK_WIDGET (priv->minimize), FALSE, FALSE, 0);
}

const gchar *
dzl_tab_get_title (DzlTab *self)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_TAB (self), NULL);

  return gtk_label_get_label (priv->title);
}

void
dzl_tab_set_title (DzlTab      *self,
                   const gchar *title)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_if_fail (DZL_IS_TAB (self));

  gtk_label_set_label (priv->title, title);
}

const gchar *
dzl_tab_get_icon_name (DzlTab *self)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);
  const gchar *ret = NULL;

  g_return_val_if_fail (DZL_IS_TAB (self), NULL);

  gtk_image_get_icon_name (priv->image, &ret, NULL);

  return ret;
}

void
dzl_tab_set_icon_name (DzlTab      *self,
                       const gchar *icon_name)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_if_fail (DZL_IS_TAB (self));

  g_object_set (priv->image, "icon-name", icon_name, NULL);
}

GtkPositionType
dzl_tab_get_edge (DzlTab *self)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_TAB (self), 0);

  return priv->edge;
}

void
dzl_tab_set_edge (DzlTab          *self,
                  GtkPositionType  edge)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_if_fail (DZL_IS_TAB (self));
  g_return_if_fail (edge >= 0);
  g_return_if_fail (edge <= 3);

  if (priv->edge != edge)
    {
      priv->edge = edge;
      dzl_tab_update_edge (self);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_EDGE]);
    }
}

/**
 * dzl_tab_get_widget:
 *
 * Returns: (transfer none) (nullable): A #GtkWidget or %NULL.
 */
GtkWidget *
dzl_tab_get_widget (DzlTab *self)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_TAB (self), NULL);

  return priv->widget;
}

void
dzl_tab_set_widget (DzlTab    *self,
                    GtkWidget *widget)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_if_fail (DZL_IS_TAB (self));

  if (dzl_set_weak_pointer (&priv->widget, widget))
    {
      gtk_label_set_mnemonic_widget (priv->title, widget);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_WIDGET]);
    }
}

gboolean
dzl_tab_get_can_close (DzlTab *self)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_TAB (self), FALSE);

  return priv->can_close;
}

void
dzl_tab_set_can_close (DzlTab   *self,
                       gboolean  can_close)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_if_fail (DZL_IS_TAB (self));

  can_close = !!can_close;

  if (can_close != priv->can_close)
    {
      priv->can_close = can_close;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_CAN_CLOSE]);
    }
}

gboolean
dzl_tab_get_active (DzlTab *self)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_TAB (self), FALSE);

  return priv->active;
}

void
dzl_tab_set_active (DzlTab   *self,
                    gboolean  active)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_if_fail (DZL_IS_TAB (self));

  active = !!active;

  if (priv->active != active)
    {
      priv->active = active;
      dzl_tab_activate (self);
      dzl_tab_apply_state (self);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ACTIVE]);
    }
}

static const gchar *
dzl_tab_get_action_name (GtkActionable *actionable)
{
  DzlTab *self = (DzlTab *)actionable;
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_TAB (self), NULL);

  return priv->action_name;
}

static void
dzl_tab_set_action_name (GtkActionable *actionable,
                         const gchar   *action_name)
{
  DzlTab *self = (DzlTab *)actionable;
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_if_fail (DZL_IS_TAB (self));

  if (g_strcmp0 (priv->action_name, action_name) != 0)
    {
      g_free (priv->action_name);
      priv->action_name = g_strdup (action_name);
      g_object_notify (G_OBJECT (self), "action-name");
    }
}

static GVariant *
dzl_tab_get_action_target_value (GtkActionable *actionable)
{
  DzlTab *self = (DzlTab *)actionable;
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_TAB (self), NULL);

  return priv->action_target_value;
}

static void
dzl_tab_set_action_target_value (GtkActionable *actionable,
                                 GVariant      *variant)
{
  DzlTab *self = (DzlTab *)actionable;
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_if_fail (DZL_IS_TAB (self));

  if (priv->action_target_value != variant)
    {
      g_clear_pointer (&priv->action_target_value, g_variant_unref);
      if (variant != NULL)
        priv->action_target_value = g_variant_ref_sink (variant);
      g_object_notify (G_OBJECT (self), "action-target");
    }
}

static void
actionable_iface_init (GtkActionableInterface *iface)
{
  iface->get_action_name = dzl_tab_get_action_name;
  iface->set_action_name = dzl_tab_set_action_name;
  iface->get_action_target_value = dzl_tab_get_action_target_value;
  iface->set_action_target_value = dzl_tab_set_action_target_value;
}

DzlTabStyle
dzl_tab_get_style (DzlTab *self)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_val_if_fail (DZL_IS_TAB (self), 0);

  return priv->style;
}

void
dzl_tab_set_style (DzlTab      *self,
                   DzlTabStyle  style)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);

  g_return_if_fail (DZL_IS_TAB (self));

  if (style != priv->style)
    {
      priv->style = style;
      gtk_widget_set_visible (GTK_WIDGET (priv->image), !!(priv->style & DZL_TAB_ICONS));
      gtk_widget_set_visible (GTK_WIDGET (priv->title), !!(priv->style & DZL_TAB_TEXT));
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_STYLE]);
    }
}

void
_dzl_tab_update_controls (DzlTab *self)
{
  DzlTabPrivate *priv = dzl_tab_get_instance_private (self);
  gboolean can_close = FALSE;
  gboolean can_minimize = FALSE;

  g_return_if_fail (DZL_IS_TAB (self));

  if (DZL_IS_DOCK_ITEM (priv->widget))
    {
      can_close = dzl_dock_item_get_can_close (DZL_DOCK_ITEM (priv->widget));
      can_minimize = dzl_dock_item_get_can_minimize (DZL_DOCK_ITEM (priv->widget));
    }

  gtk_widget_set_visible (GTK_WIDGET (priv->close), can_close);
  gtk_widget_set_visible (GTK_WIDGET (priv->minimize), can_minimize);
}

GType
dzl_tab_style_get_type (void)
{
  static GType type_id;

  if (g_once_init_enter (&type_id))
    {
      GType _type_id;
      static const GFlagsValue values[] = {
        { DZL_TAB_ICONS, "DZL_TAB_ICONS", "icons" },
        { DZL_TAB_TEXT, "DZL_TAB_TEXT", "text" },
        { DZL_TAB_BOTH, "DZL_TAB_BOTH", "both" },
        { 0 }
      };

      _type_id = g_flags_register_static ("DzlTabStyle", values);
      g_once_init_leave (&type_id, _type_id);
    }

  return type_id;
}
