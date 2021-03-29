/*
 * Copyright (C) 2019 Alexandros Theodotou
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

#include "gui/widgets/slider_bar.h"

G_DEFINE_TYPE (SliderBarWidget,
               slider_bar_widget,
               GTK_TYPE_DRAWING_AREA)

static int
slider_bar_draw_cb (
  GtkWidget * widget,
  cairo_t * cr,
  SliderBarWidget * self)
{
  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  cairo_set_source_rgba (cr, 1.0, 0.3, 0.3, 1.0);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);

  return FALSE;
}

/**
 * Creates a new SliderBarWidget.
 *
 * @param width -1 if not fixed.
 * @param height -1 if not fixed.
 */
SliderBarWidget *
_slider_bar_widget_new (
  float (*get_val) (void *),
  void (*set_val) (void *, float),
  void * object,
  float min,
  float max,
  int   width,
  int   height,
  float zero,
  const char * text)
{
  SliderBarWidget * self =
    g_object_new (SLIDER_BAR_WIDGET_TYPE, NULL);

  g_warn_if_fail (object);

  self->getter = get_val;
  self->setter = set_val;
  self->object = object;
  self->min = min;
  self->max = max;
  self->width = width;
  self->height = height;
  self->zero = zero;

  /* FIXME free it */
  self->text = g_strdup (text);

  gtk_widget_set_size_request (
    GTK_WIDGET (self), width, height);

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (slider_bar_draw_cb), self);

  return self;
}

static void
slider_bar_widget_init (
  SliderBarWidget * self)
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
      gtk_gesture_drag_new (
        GTK_WIDGET (self)));

  gtk_widget_set_visible (
    GTK_WIDGET (self), 1);
}

static void
slider_bar_widget_class_init (
  SliderBarWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "slider-bar");
}
