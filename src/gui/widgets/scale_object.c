/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/scale_object.h"
#include "audio/chord_track.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/scale_selector_window.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/ui.h"

G_DEFINE_TYPE (
  ScaleObjectWidget,
  scale_object_widget,
  GTK_TYPE_BOX)

static gboolean
scale_draw_cb (
  GtkWidget * widget,
  cairo_t *   cr,
  ScaleObjectWidget * self)
{
  guint width, height;
  GtkStyleContext *context;

  context =
    gtk_widget_get_style_context (widget);

  width =
    gtk_widget_get_allocated_width (widget);
  height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  GdkRGBA * color = &P_CHORD_TRACK->color;
  cairo_set_source_rgba (
    cr, color->red, color->green, color->blue,
    scale_object_is_transient (self->scale_object) ?
      0.7 : 1);
  if (scale_object_is_selected (self->scale_object))
    {
      cairo_set_source_rgba (
        cr, color->red + 0.4, color->green + 0.2,
        color->blue + 0.2, 1);
    }
  cairo_rectangle (
    cr, 0, 0,
    width - SCALE_OBJECT_WIDGET_TRIANGLE_W, height);
  cairo_fill(cr);

  cairo_move_to (
    cr, width - SCALE_OBJECT_WIDGET_TRIANGLE_W, 0);
  cairo_line_to (
    cr, width, height);
  cairo_line_to (
    cr, width - SCALE_OBJECT_WIDGET_TRIANGLE_W,
    height);
  cairo_line_to (
    cr, width - SCALE_OBJECT_WIDGET_TRIANGLE_W, 0);
  cairo_close_path (cr);
  cairo_fill(cr);

  char * str =
    musical_scale_to_string (
      self->scale_object->scale);
  if (DEBUGGING &&
      scale_object_is_transient (self->scale_object))
    {
      char * tmp =
        g_strdup_printf (
          "%s [t]", str);
      g_free (str);
      str = tmp;
    }

  GdkRGBA c2;
  ui_get_contrast_text_color (
    color, &c2);
  cairo_set_source_rgba (
    cr, c2.red, c2.green, c2.blue, 1.0);
  z_cairo_draw_text (cr, widget, str);
  g_free (str);

 return FALSE;
}

static void
on_press (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  ScaleObjectWidget *   self)
{
  if (n_press == 2)
    {
      ScaleSelectorWindowWidget * scale_selector =
        scale_selector_window_widget_new (
          self);

      gtk_window_present (
        GTK_WINDOW (scale_selector));
    }
}

/**
 * Sets the "selected" GTK state flag and adds the
 * note to TimelineSelections.
 */
void
scale_object_widget_select (
  ScaleObjectWidget * self,
  int              select)
{
  ScaleObject * main_scale =
    scale_object_get_main_scale_object (
      self->scale_object);
  if (select)
    {
      timeline_selections_add_scale_object (
        TL_SELECTIONS,
        main_scale);
    }
  else
    {
      timeline_selections_remove_scale_object (
        TL_SELECTIONS,
        main_scale);
    }
  EVENTS_PUSH (ET_SCALE_OBJECT_CHANGED,
               main_scale);
}

static gboolean
on_motion (
  GtkWidget *      widget,
  GdkEventMotion * event,
  ScaleObjectWidget * self)
{
  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT,
        0);
      bot_bar_change_status (
        "Scale - Click and drag to move around ("
        "hold Shift to disable snapping)");
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);
      bot_bar_change_status ("");
    }

  return FALSE;
}

ScaleObjectWidget *
scale_object_widget_new (ScaleObject * scale)
{
  ScaleObjectWidget * self =
    g_object_new (SCALE_OBJECT_WIDGET_TYPE, NULL);

  self->scale_object = scale;

  return self;
}

static void
scale_object_widget_class_init (
  ScaleObjectWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  /* "scale" is taken by gtk */
  gtk_widget_class_set_css_name (
    klass, "scale-object");
}

static void
scale_object_widget_init (
  ScaleObjectWidget * self)
{
  self->drawing_area =
    GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->drawing_area), 1);
  gtk_widget_set_hexpand (
    GTK_WIDGET (self->drawing_area), 1);
  gtk_container_add (
    GTK_CONTAINER (self),
    GTK_WIDGET (self->drawing_area));

  /* GDK_ALL_EVENTS_MASK is needed, otherwise the
   * grab gets broken */
  gtk_widget_add_events (
    GTK_WIDGET (self->drawing_area),
    GDK_ALL_EVENTS_MASK);

  self->mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self->drawing_area)));

  g_signal_connect (
    G_OBJECT (self->drawing_area), "draw",
    G_CALLBACK (scale_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self->drawing_area),
    "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self->drawing_area),
    "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self->drawing_area),
    "motion-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (self->mp), "pressed",
    G_CALLBACK (on_press), self);

  g_object_ref (self);
}
