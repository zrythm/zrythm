/*
 * Copyright (C) 2019, 2021 Alexandros Theodotou <alex at zrythm dot org>
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

G_DEFINE_TYPE (
  SliderBarWidget,
  slider_bar_widget,
  GTK_TYPE_DRAWING_AREA)

static void
slider_bar_draw_cb (
  GtkDrawingArea * drawing_area,
  cairo_t *        cr,
  int              width,
  int              height,
  gpointer         user_data)
{
  SliderBarWidget * self =
    Z_SLIDER_BAR_WIDGET (user_data);
  GtkWidget * widget = GTK_WIDGET (self);

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  cairo_set_source_rgba (cr, 1.0, 0.3, 0.3, 1.0);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);
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
  void *       object,
  float        min,
  float        max,
  int          width,
  int          height,
  float        zero,
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

  gtk_drawing_area_set_draw_func (
    GTK_DRAWING_AREA (self), slider_bar_draw_cb,
    self, NULL);

  return self;
}

static void
slider_bar_widget_init (SliderBarWidget * self)
{
  self->drag =
    GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->drag));
}

static void
slider_bar_widget_class_init (
  SliderBarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "slider-bar");
}
