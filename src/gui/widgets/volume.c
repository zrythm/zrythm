/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <math.h>

#include "audio/control_port.h"
#include "audio/port.h"
#include "gui/widgets/volume.h"
#include "utils/ui.h"

G_DEFINE_TYPE (
  VolumeWidget, volume_widget,
  GTK_TYPE_DRAWING_AREA)

#define PADDING 6

/**
 * Draws the volume.
 */
static int
volume_draw_cb (
  GtkWidget *    widget,
  cairo_t *      cr,
  VolumeWidget * self)
{
  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  double density = 0.85;
  if (self->hover)
    density = 1;

  cairo_set_source_rgba (
    cr, density, density, density, 1);
  cairo_set_line_width (cr, 2);

  /* draw outline */
  cairo_move_to (cr, 0, height);
  cairo_line_to (cr, width, height);
  cairo_line_to (cr, width, 0);
  cairo_close_path (cr);
  cairo_stroke (cr);

  /* draw filled in bar */
  double normalized_val =
    control_port_get_normalized_val (self->port);
  double filled_w =
    normalized_val * (double) width;
  double filled_h =
    /* tan (theta) * filled_w */
    ((double) height / (double) width) * filled_w;
  cairo_move_to (cr, 0, height);
  cairo_line_to (cr, filled_w, height);
  cairo_line_to (cr, filled_w, height - filled_h);
  cairo_close_path (cr);
  cairo_fill (cr);

  return FALSE;
}

static void
on_crossing (
  GtkWidget *    widget,
  GdkEvent *     event,
  VolumeWidget * self)
{
  if (event->type == GDK_ENTER_NOTIFY)
    {
      self->hover = 1;
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      if (!gtk_gesture_drag_get_offset (
             self->drag, NULL, NULL))
        self->hover = 0;
    }
  gtk_widget_queue_draw (widget);
}

static void
drag_update (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  VolumeWidget *   self)
{
  offset_y = - offset_y;
  const int use_y =
    fabs (offset_y - self->last_y) >
    fabs (offset_x - self->last_x);
  const float cur_norm_val =
    control_port_get_normalized_val (self->port);
  const float delta =
    use_y ?
      (float) (offset_y - self->last_y) :
      (float) (offset_x - self->last_x);
  const float multiplier = 0.012f;
  const float new_norm_val =
    CLAMP (
      cur_norm_val + multiplier * delta,
       0.f, 1.f);
  control_port_set_val_from_normalized (
    self->port, new_norm_val, false);

  gtk_widget_queue_draw (GTK_WIDGET (self));

  self->last_x = offset_x;
  self->last_y = offset_y;
}

static void
drag_end (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  VolumeWidget *   self)
{
  GdkModifierType state_mask =
    ui_get_state_mask (GTK_GESTURE (gesture));

  /* reset if ctrl-clicked */
  if (state_mask & GDK_CONTROL_MASK)
    {
      float def_val = self->port->deff;
      control_port_set_real_val (
        self->port, def_val);
    }

  self->last_x = 0;
  self->last_y = 0;
}

void
volume_widget_setup (
  VolumeWidget * self,
  Port *         port)
{
  self->port = port;

  self->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (GTK_WIDGET (self)));

  g_signal_connect (
    G_OBJECT(self->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_crossing),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_crossing),  self);
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (volume_draw_cb), self);
}

VolumeWidget *
volume_widget_new (
  Port * port)
{
  VolumeWidget * self =
    g_object_new (VOLUME_WIDGET_TYPE, NULL);

  volume_widget_setup (self, port);

  return self;
}

static void
volume_widget_class_init (
  VolumeWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);

  gtk_widget_class_set_css_name (
    klass, "volume");
}

static void
volume_widget_init (
  VolumeWidget * self)
{
  /* make it able to notify */
  gtk_widget_set_has_window (
    GTK_WIDGET (self), TRUE);
  int crossing_mask =
    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
  gtk_widget_add_events (
    GTK_WIDGET (self), crossing_mask);

  gtk_widget_set_margin_start (
    GTK_WIDGET (self), PADDING);
  gtk_widget_set_margin_end (
    GTK_WIDGET (self), PADDING);
  gtk_widget_set_margin_bottom (
    GTK_WIDGET (self), PADDING);
  gtk_widget_set_margin_top (
    GTK_WIDGET (self), PADDING);
}
