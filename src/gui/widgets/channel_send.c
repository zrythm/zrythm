// SPDX-FileCopyrightText: © 2020-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/channel_send.h"
#include "gui/widgets/channel_send.h"
#include "gui/widgets/channel_send_selector.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ChannelSendWidget,
  channel_send_widget,
  GTK_TYPE_WIDGET)

#define ELLIPSIZE_PADDING 2

static void
update_pango_layout (ChannelSendWidget * self)
{
  bool empty = channel_send_is_empty (self->send);
  bool changed = false;
  if (empty != self->was_empty)
    {
      changed = true;
      self->was_empty = empty;
      if (empty)
        gtk_widget_add_css_class (GTK_WIDGET (self), "empty");
      else
        gtk_widget_remove_css_class (
          GTK_WIDGET (self), "empty");
    }

  if (!self->txt_layout || changed)
    {
      object_free_w_func_and_null (
        g_object_unref, self->txt_layout);
      PangoLayout * layout = gtk_widget_create_pango_layout (
        GTK_WIDGET (self), NULL);
      pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

      self->txt_layout = layout;
    }

  pango_layout_set_width (
    self->txt_layout,
    pango_units_from_double (MAX (
      gtk_widget_get_allocated_width (GTK_WIDGET (self))
        - ELLIPSIZE_PADDING * 2,
      1)));
}

static void
channel_send_snapshot (
  GtkWidget *   widget,
  GtkSnapshot * snapshot)
{
  ChannelSendWidget * self = Z_CHANNEL_SEND_WIDGET (widget);

  int width = gtk_widget_get_allocated_width (widget);
  int height = gtk_widget_get_allocated_height (widget);

  update_pango_layout (self);

  GdkRGBA fg;
  gtk_widget_get_color (GTK_WIDGET (self), &fg);
  int           padding = 2;
  ChannelSend * send = self->send;
  char          dest_name[400] = "";
  channel_send_get_dest_name (send, dest_name);
  if (channel_send_is_empty (send))
    {
      /* fill background */
      gtk_snapshot_append_color (
        snapshot, &Z_GDK_RGBA_INIT (0.1f, 0.1f, 0.1f, 1.f),
        &GRAPHENE_RECT_INIT (
          (float) padding, (float) padding,
          (float) width - (float) padding * 2.f,
          (float) height - (float) padding * 2.f));

      /* fill text */
      int w, h;
      pango_layout_set_markup (
        self->txt_layout, dest_name, -1);
      pango_layout_get_pixel_size (self->txt_layout, &w, &h);
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (
          (float) (width / 2 - w / 2),
          (float) (height / 2 - h / 2)));
      gtk_snapshot_append_layout (
        snapshot, self->txt_layout, &fg);
      gtk_snapshot_restore (snapshot);

      /* update tooltip */
      if (self->cache_tooltip)
        {
          g_free (self->cache_tooltip);
          self->cache_tooltip = NULL;
          gtk_widget_set_tooltip_text (widget, dest_name);
        }
    }
  else
    {
      GdkRGBA bg;
      if (channel_send_is_prefader (self->send))
        bg = UI_COLORS->prefader_send;
      else
        bg = UI_COLORS->postfader_send;

      /* fill background */
      GdkRGBA bg_color = bg;
      bg_color.alpha = 0.4f;
      gtk_snapshot_append_color (
        snapshot, &bg_color,
        &GRAPHENE_RECT_INIT (
          (float) padding, (float) padding,
          (float) width - (float) padding * 2.f,
          (float) height - (float) padding * 2.f));

      /* fill amount */
      bg_color.alpha = 1.f;
      float amount =
        channel_send_get_amount_for_widgets (self->send)
        * (float) width;
      gtk_snapshot_append_color (
        snapshot, &bg_color,
        &GRAPHENE_RECT_INIT (
          (float) padding, (float) padding,
          amount - (float) padding * 2.f,
          (float) height - (float) padding * 2));

      /* fill text */
      int w, h;
      pango_layout_set_markup (
        self->txt_layout, dest_name, -1);
      pango_layout_get_pixel_size (self->txt_layout, &w, &h);
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (
          (float) (width / 2 - w / 2),
          (float) (height / 2 - h / 2)));
      gtk_snapshot_append_layout (
        snapshot, self->txt_layout, &fg);
      gtk_snapshot_restore (snapshot);

      /* update tooltip */
      if (
        !self->cache_tooltip
        || !g_strcmp0 (dest_name, self->cache_tooltip))
        {
          if (self->cache_tooltip)
            g_free (self->cache_tooltip);
          self->cache_tooltip = g_strdup (dest_name);
          gtk_widget_set_tooltip_text (
            widget, self->cache_tooltip);
        }
    }
}

static void
on_drag_begin (
  GtkGestureDrag *    gesture,
  gdouble             start_x,
  gdouble             start_y,
  ChannelSendWidget * self)
{
  self->start_x = start_x;
  self->send_amount_at_start = self->send->amount->control;
}

static void
on_drag_update (
  GtkGestureDrag *    gesture,
  gdouble             offset_x,
  gdouble             offset_y,
  ChannelSendWidget * self)
{
  if (channel_send_is_enabled (self->send))
    {
      int width =
        gtk_widget_get_allocated_width (GTK_WIDGET (self));
      double new_normalized_val =
        ui_get_normalized_draggable_value (
          width,
          channel_send_get_amount_for_widgets (self->send),
          self->start_x, self->start_x + offset_x,
          self->start_x + self->last_offset_x, 1.0,
          UI_DRAG_MODE_CURSOR);
      channel_send_set_amount_from_widget (
        self->send, (float) new_normalized_val);
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }

  self->last_offset_x = offset_x;
}

static void
on_drag_end (
  GtkGestureDrag *    gesture,
  gdouble             offset_x,
  gdouble             offset_y,
  ChannelSendWidget * self)
{
  self->start_x = 0;
  self->last_offset_x = 0;

  float send_amount_at_end = self->send->amount->control;
  port_set_control_value (
    self->send->amount, self->send_amount_at_start,
    F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);

  if (channel_send_is_enabled (self->send) && self->n_press != 2)
    {
      GError * err = NULL;
      bool ret = channel_send_action_perform_change_amount (
        self->send, send_amount_at_end, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Failed to change send amount"));
        }
    }
}

static void
on_pressed (
  GtkGestureClick *   gesture,
  gint                n_press,
  gdouble             x,
  gdouble             y,
  ChannelSendWidget * self)
{
  self->n_press = n_press;
  g_message ("clicked %d", n_press);

  if (n_press == 2)
    {
      channel_send_selector_widget_setup (
        self->selector_popover);
      gtk_popover_popup (GTK_POPOVER (self->selector_popover));
    }
}

static void
show_context_menu (ChannelSendWidget * self, double x, double y)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  char tmp[500];
  sprintf (tmp, "app.bind-midi-cc::%p", self->send->amount);
  menuitem = CREATE_MIDI_LEARN_MENU_ITEM (tmp);
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x, y, menu);
}

static void
on_right_click (
  GtkGestureClick *   gesture,
  gint                n_press,
  gdouble             x,
  gdouble             y,
  ChannelSendWidget * self)
{
  if (n_press != 1)
    return;

  show_context_menu (self, x, y);
}

static void
on_enter (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  gpointer                   user_data)
{
  ChannelSendWidget * self = Z_CHANNEL_SEND_WIDGET (user_data);
  gtk_widget_set_state_flags (
    GTK_WIDGET (self), GTK_STATE_FLAG_PRELIGHT, 0);
}

static void
on_leave (
  GtkEventControllerMotion * motion_controller,
  gpointer                   user_data)
{
  ChannelSendWidget * self = Z_CHANNEL_SEND_WIDGET (user_data);
  gtk_widget_unset_state_flags (
    GTK_WIDGET (self), GTK_STATE_FLAG_PRELIGHT);
}

static void
dispose (ChannelSendWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));
  gtk_widget_unparent (GTK_WIDGET (self->selector_popover));

  G_OBJECT_CLASS (channel_send_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
finalize (ChannelSendWidget * self)
{
  if (self->cache_tooltip)
    g_free (self->cache_tooltip);
  if (self->txt_layout)
    g_object_unref (self->txt_layout);

  G_OBJECT_CLASS (channel_send_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

/**
 * Creates a new ChannelSend widget and binds it to
 * the given value.
 */
ChannelSendWidget *
channel_send_widget_new (ChannelSend * send)
{
  ChannelSendWidget * self =
    g_object_new (CHANNEL_SEND_WIDGET_TYPE, NULL);
  self->send = send;

  return self;
}

static void
channel_send_widget_init (ChannelSendWidget * self)
{
  gtk_widget_set_size_request (GTK_WIDGET (self), -1, 20);

  self->cache_tooltip = NULL;

  gtk_widget_set_hexpand (GTK_WIDGET (self), true);
  gtk_widget_set_focusable (GTK_WIDGET (self), true);

  self->selector_popover =
    channel_send_selector_widget_new (self);
  gtk_widget_set_parent (
    GTK_WIDGET (self->selector_popover), GTK_WIDGET (self));

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
  gtk_widget_set_parent (
    GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  self->click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->click), GTK_PHASE_CAPTURE);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->click));

  self->right_mouse_mp =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_mouse_mp),
    GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->right_mouse_mp));

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->drag), GTK_PHASE_CAPTURE);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag));

  GtkEventController * motion_controller =
    gtk_event_controller_motion_new ();
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (motion_controller));

  /* connect signals */
  g_signal_connect (
    G_OBJECT (motion_controller), "enter",
    G_CALLBACK (on_enter), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave",
    G_CALLBACK (on_leave), self);
  g_signal_connect (
    G_OBJECT (self->click), "pressed",
    G_CALLBACK (on_pressed), self);
  g_signal_connect (
    G_OBJECT (self->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin",
    G_CALLBACK (on_drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update",
    G_CALLBACK (on_drag_update), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end",
    G_CALLBACK (on_drag_end), self);
}

static void
channel_send_widget_class_init (ChannelSendWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  gtk_widget_class_set_css_name (wklass, "channel-send");
  wklass->snapshot = channel_send_snapshot;

  gtk_widget_class_set_layout_manager_type (
    wklass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
