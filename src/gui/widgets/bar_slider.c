/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "audio/port.h"
#include "gui/widgets/bar_slider.h"
#include "utils/cairo.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  BarSliderWidget,
  bar_slider_widget,
  GTK_TYPE_DRAWING_AREA)

/**
 * Macro to get real value.
 */
#define GET_REAL_VAL \
  ((*self->getter) (self->object))

/**
 * Macro to get real value from bar_slider value.
 */
#define REAL_VAL_FROM_BAR_SLIDER(bar_slider) \
  (self->min + bar_slider * \
   (self->max - self->min))

/**
 * Converts from real value to bar_slider value
 */
#define BAR_SLIDER_VAL_FROM_REAL(real) \
  ((real - self->min) / \
   (self->max - self->min))

/**
 * Sets real val
 */
#define SET_REAL_VAL(real) \
  ((*self->setter)(self->object, real))

/**
 * Draws the bar_slider.
 */
static int
draw_cb (
  GtkWidget * widget,
  cairo_t * cr,
  BarSliderWidget * self)
{
  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  guint width, height;
  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  const float real_min = self->min;
  const float real_max = self->max;
  const float real_zero = self->zero;
  const float real_val = GET_REAL_VAL;

  /* get absolute values in pixels */
  const float min_px = 0.f;
  const float max_px = width;
  const float zero_px =
    ((real_zero - real_min) /
     (real_max - real_min)) *
    width;
  const float val_px =
    ((real_val - real_min) /
     (real_max - real_min)) *
    width;

  cairo_set_source_rgba (cr, 1, 1, 1, 0.3);

  /* draw from val to zero */
  if (real_val < real_zero)
    {
      cairo_rectangle (
        cr, val_px, 0, zero_px, height);
    }
  /* draw from zero to val */
  else
    {
      cairo_rectangle (
        cr, zero_px, 0, val_px, height);
    }
  cairo_fill (cr);

  char * str;
  if (self->decimals == 0)
    {
      str =
        g_strdup_printf (
          "%d%s", (int) real_val, self->suffix);
    }
    else
    {
      str =
        g_strdup_printf (
          "%f%s", real_val, self->suffix);
    }
  int we;
  cairo_set_source_rgba (cr, 1, 1, 1, 1);
  z_cairo_get_text_extents_for_widget_full (
    GTK_WIDGET (self), str, &we, NULL,
    Z_CAIRO_FONT);
  z_cairo_draw_text_full (
    cr, str, width / 2 - we / 2,
    Z_CAIRO_TEXT_PADDING, Z_CAIRO_FONT);

  if (self->hover)
    {
      cairo_set_source_rgba (cr, 1,1,1, 0.12 );
      cairo_rectangle (cr, 0, 0, width, height);
      cairo_fill (cr);
    }

  return FALSE;
}

static void
on_crossing (
  GtkWidget *       widget,
  GdkEvent *        event,
  BarSliderWidget * self)
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

static double clamp
(double x, double upper, double lower)
{
   return MIN(upper, MAX(x, lower));
}

static void
drag_begin (
  GtkGestureDrag * gesture,
  double           offset_x,
  double           offset_y,
  BarSliderWidget * self)
{
  if (self->mode == BAR_SLIDER_UPDATE_MODE_CURSOR)
    {
      SET_REAL_VAL (
        REAL_VAL_FROM_BAR_SLIDER (
          clamp (
            offset_x /
              gtk_widget_get_allocated_width (
                GTK_WIDGET (self)),
            1.0f, 0.0f)));
      self->start_x = offset_x;
      gtk_widget_queue_draw ((GtkWidget *)self);
    }
}

static void
drag_update (
  GtkGestureDrag *gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  BarSliderWidget * self)
{
  if (self->mode == BAR_SLIDER_UPDATE_MODE_CURSOR)
    {
      SET_REAL_VAL (
        REAL_VAL_FROM_BAR_SLIDER (
          clamp (
            (self->start_x + offset_x) /
              gtk_widget_get_allocated_width (
                GTK_WIDGET (self)),
            1.0f, 0.0f)));
    }
  else if (self->mode ==
             BAR_SLIDER_UPDATE_MODE_RELATIVE)
    {
      SET_REAL_VAL (
        REAL_VAL_FROM_BAR_SLIDER (
          clamp (
            BAR_SLIDER_VAL_FROM_REAL (GET_REAL_VAL) +
              (offset_x - self->last_x) /
              gtk_widget_get_allocated_width (
                GTK_WIDGET (self)),
            1.0f, 0.0f)));
      self->last_x = offset_x;
    }
  gtk_widget_queue_draw ((GtkWidget *)self);
}

static void
drag_end (
  GtkGestureDrag *gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  BarSliderWidget * self)
{
  self->last_x = 0;
  self->start_x = 0;
}

/**
 * Creates a bar slider widget for floats.
 */
BarSliderWidget *
_bar_slider_widget_new (
  float (*get_val)(void *),
  void (*set_val)(void *, float),
  void * object,
  float    min,
  float    max,
  int    w,
  int    h,
  float    zero,
  int       decimals,
  BarSliderUpdateMode mode,
  const char * suffix)
{
  g_warn_if_fail (object);

  BarSliderWidget * self =
    g_object_new (BAR_SLIDER_WIDGET_TYPE, NULL);
  self->getter = get_val;
  self->setter = set_val;
  self->object = object;
  self->min = min;
  self->max = max;
  self->hover = 0;
  self->zero = zero; /* default 0.05f */
  self->last_x = 0;
  self->start_x = 0;
  self->suffix = g_strdup (suffix);
  self->decimals = decimals;
  self->mode = mode;

  /* set size */
  gtk_widget_set_size_request (
    GTK_WIDGET (self), w, h);
  gtk_widget_set_hexpand (
    GTK_WIDGET (self), 1);
  gtk_widget_set_vexpand (
    GTK_WIDGET (self), 1);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (draw_cb), self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_crossing),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_crossing),  self);
  if (mode == BAR_SLIDER_UPDATE_MODE_CURSOR)
    {
      g_signal_connect (
        G_OBJECT(self->drag), "drag-begin",
        G_CALLBACK (drag_begin),  self);
    }
  g_signal_connect (
    G_OBJECT(self->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  return self;
}

static void
bar_slider_widget_init (
  BarSliderWidget * self)
{
  /* make it able to notify */
  gtk_widget_set_has_window (
    GTK_WIDGET (self), TRUE);
  int crossing_mask =
    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
  gtk_widget_add_events (
    GTK_WIDGET (self), crossing_mask);
  self->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (GTK_WIDGET (self)));

  gtk_widget_set_visible (
    GTK_WIDGET (self), 1);
}

static void
bar_slider_widget_class_init (
  BarSliderWidgetClass * klass)
{
}
