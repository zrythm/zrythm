/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/channel_send.h"
#include "gui/widgets/channel_send.h"
#include "gui/widgets/channel_send_selector.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/gtk.h"
#include "utils/flags.h"
#include "utils/ui.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ChannelSendWidget, channel_send_widget,
  GTK_TYPE_DRAWING_AREA)

#define ELLIPSIZE_PADDING 2

static int
channel_send_draw_cb (
  GtkWidget *         widget,
  cairo_t *           cr,
  ChannelSendWidget * self)
{
  GtkStyleContext *context =
  gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  int padding = 2;
  ChannelSend * send = self->send;
  char dest_name[400];
  channel_send_get_dest_name (send, dest_name);
  if (send->is_empty)
    {
      /* fill background */
      cairo_set_source_rgba (
        cr, 0.1, 0.1, 0.1, 1.0);
      cairo_rectangle (
        cr, padding, padding,
        width - padding * 2, height - padding * 2);
      cairo_fill(cr);

      /* fill text */
      cairo_set_source_rgba (
        cr, 0.3, 0.3, 0.3, 1.0);
      int w, h;
      z_cairo_get_text_extents_for_widget (
        widget, self->empty_slot_layout, dest_name,
        &w, &h);
      z_cairo_draw_text_full (
        cr, widget, self->empty_slot_layout,
        dest_name, width / 2 - w / 2,
        height / 2 - h / 2);

      /* update tooltip */
      if (self->cache_tooltip)
        {
          g_free (self->cache_tooltip);
          self->cache_tooltip = NULL;
          gtk_widget_set_tooltip_text (
            widget, dest_name);
        }
    }
  else
    {
      GdkRGBA bg, fg;
      fg = UI_COLOR_BLACK;
      if (channel_send_is_prefader (self->send))
        bg = UI_COLORS->prefader_send;
      else
        bg = UI_COLORS->postfader_send;

      /* fill background */
      cairo_set_source_rgba (
        cr, bg.red, bg.green, bg.blue, 0.4);
      cairo_rectangle (
        cr, padding, padding,
        width - padding * 2, height - padding * 2);
      cairo_fill(cr);

      /* fill amount */
      float amount =
        channel_send_get_amount_for_widgets (
          self->send) * width;
      cairo_set_source_rgba (
        cr, bg.red, bg.green, bg.blue, 1.0);
      cairo_rectangle (
        cr, padding, padding,
        amount - padding * 2, height - padding * 2);
      cairo_fill(cr);

      /* fill text */
      cairo_set_source_rgba (
        cr, fg.red, fg.green, fg.blue, 1.0);
      int w, h;
      z_cairo_get_text_extents_for_widget (
        widget, self->name_layout,
        dest_name, &w, &h);
      z_cairo_draw_text_full (
        cr, widget, self->name_layout,
        dest_name,
        width / 2 - w / 2,
        height / 2 - h / 2);

      /* update tooltip */
      if (!self->cache_tooltip ||
          !g_strcmp0 (
            dest_name, self->cache_tooltip))
        {
          if (self->cache_tooltip)
            g_free (self->cache_tooltip);
          self->cache_tooltip =
            g_strdup (dest_name);
          gtk_widget_set_tooltip_text (
            widget, self->cache_tooltip);
        }
    }

  return FALSE;
}

static void
on_drag_begin (
  GtkGestureDrag *    gesture,
  gdouble             start_x,
  gdouble             start_y,
  ChannelSendWidget * self)
{
  self->start_x = start_x;
  self->send_amount_at_start = self->send->amount;
}

static void
on_drag_update (
  GtkGestureDrag *    gesture,
  gdouble             offset_x,
  gdouble             offset_y,
  ChannelSendWidget * self)
{
  if (!self->send->is_empty)
    {
      int width =
        gtk_widget_get_allocated_width (
          GTK_WIDGET (self));
      double new_normalized_val =
        ui_get_normalized_draggable_value (
          width,
          channel_send_get_amount_for_widgets (
            self->send),
          self->start_x, self->start_x + offset_x,
          self->start_x + self->last_offset_x,
          1.0, UI_DRAG_MODE_CURSOR);
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

  float send_amount_at_end = self->send->amount;
  self->send->amount = self->send_amount_at_start;

  if (!self->send->is_empty && self->n_press != 2)
    {
      UndoableAction * ua =
        channel_send_action_new_change_amount (
          self->send, send_amount_at_end);
      undo_manager_perform (UNDO_MANAGER, ua);
    }
}

static void
multipress_pressed (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  ChannelSendWidget *   self)
{
  self->n_press = n_press;
  g_message ("multipress %d", n_press);

  if (n_press == 2)
    {
      ChannelSendSelectorWidget * send_selector =
        channel_send_selector_widget_new (
          self);
      gtk_widget_show_all (
        GTK_WIDGET (send_selector));
    }
}

static void
show_context_menu (
  ChannelSendWidget * self)
{
  GtkWidget *menu;
  GtkMenuItem * menuitem;

  menu = gtk_menu_new();

#define CREATE_SEPARATOR \
  menuitem = \
    GTK_MENU_ITEM (gtk_separator_menu_item_new ()); \
  gtk_widget_show_all (GTK_WIDGET (menuitem))

#define ADD_TO_SHELL \
  gtk_menu_shell_append ( \
    GTK_MENU_SHELL(menu), GTK_WIDGET (menuitem))

  menuitem = CREATE_SELECT_ALL_MENU_ITEM (NULL);
  ADD_TO_SHELL;

#undef ADD_TO_SHELL

  gtk_widget_show_all(menu);
  gtk_menu_attach_to_widget (
    GTK_MENU (menu),
    GTK_WIDGET (self), NULL);

  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static void
on_right_click (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  ChannelSendWidget *   self)
{
  if (n_press != 1)
    return;

  show_context_menu (self);
}

static gboolean
on_motion (
  GtkWidget * widget,
  GdkEvent *event,
  ChannelSendWidget * self)
{
  if (gdk_event_get_event_type (event) ==
        GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT, 0);
    }
  else if (gdk_event_get_event_type (event) ==
             GDK_LEAVE_NOTIFY)
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);
    }

  return FALSE;
}

static void
recreate_pango_layouts (
  ChannelSendWidget * self)
{
  if (PANGO_IS_LAYOUT (self->empty_slot_layout))
    g_object_unref (self->empty_slot_layout);
  if (PANGO_IS_LAYOUT (self->name_layout))
    g_object_unref (self->name_layout);

  self->empty_slot_layout =
    z_cairo_create_pango_layout_from_string (
      (GtkWidget *) self, "Arial Italic 7.5",
      PANGO_ELLIPSIZE_END, ELLIPSIZE_PADDING);
  self->name_layout =
    z_cairo_create_pango_layout_from_string (
      (GtkWidget *) self, "Arial Bold 7.5",
      PANGO_ELLIPSIZE_END, ELLIPSIZE_PADDING);
}

static void
on_size_allocate (
  GtkWidget *          widget,
  GdkRectangle *       allocation,
  ChannelSendWidget * self)
{
  recreate_pango_layouts (self);
}

static void
on_screen_changed (
  GtkWidget *          widget,
  GdkScreen *          previous_screen,
  ChannelSendWidget * self)
{
  recreate_pango_layouts (self);
}

static void
finalize (
  ChannelSendWidget * self)
{
  if (self->cache_tooltip)
    g_free (self->cache_tooltip);
  if (self->drag)
    g_object_unref (self->drag);
  if (self->empty_slot_layout)
    g_object_unref (self->empty_slot_layout);
  if (self->name_layout)
    g_object_unref (self->name_layout);

  G_OBJECT_CLASS (
    channel_send_widget_parent_class)->
      finalize (G_OBJECT (self));
}

/**
 * Creates a new ChannelSend widget and binds it to
 * the given value.
 */
ChannelSendWidget *
channel_send_widget_new (
  ChannelSend * send)
{
  ChannelSendWidget * self =
    g_object_new (
      CHANNEL_SEND_WIDGET_TYPE, NULL);
  self->send = send;

  return self;
}

static void
channel_send_widget_init (
  ChannelSendWidget * self)
{
  /* make it able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (self),
    GDK_BUTTON_PRESS_MASK |
    GDK_ENTER_NOTIFY_MASK |
    GDK_LEAVE_NOTIFY_MASK);

  gtk_widget_set_size_request (
    GTK_WIDGET (self), -1, 20);

  self->cache_tooltip = NULL;

  self->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->multipress),
    GTK_PHASE_CAPTURE);
  self->right_mouse_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (
      self->right_mouse_mp),
      GDK_BUTTON_SECONDARY);
  self->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (GTK_WIDGET (self)));
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->drag),
    GTK_PHASE_CAPTURE);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (channel_send_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (self->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (
    G_OBJECT (self->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin",
    G_CALLBACK (on_drag_begin),  self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update",
    G_CALLBACK (on_drag_update),  self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end",
    G_CALLBACK (on_drag_end),  self);
  g_signal_connect (
    G_OBJECT (self), "screen-changed",
    G_CALLBACK (on_screen_changed),  self);
  g_signal_connect (
    G_OBJECT (self), "size-allocate",
    G_CALLBACK (on_size_allocate),  self);
}

static void
channel_send_widget_class_init (
  ChannelSendWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "channel-send");

  GObjectClass * oklass =
    G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) finalize;
}
