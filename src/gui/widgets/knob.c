// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2010 Paul Davis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "dsp/port.h"
#include "dsp/port_connection.h"
#include "gui/widgets/knob.h"
#include "utils/cairo.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/string.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (KnobWidget, knob_widget, GTK_TYPE_WIDGET)

#define ARC_CUT_ANGLE 60.f

static float
get_real_val (KnobWidget * self, bool snapped)
{
  switch (self->type)
    {
    case KNOB_TYPE_NORMAL:
      if (snapped && self->snapped_getter)
        {
          return self->snapped_getter (self->object);
        }
      else
        {
          return self->getter (self->object);
        }
    case KNOB_TYPE_PORT_MULTIPLIER:
      {
        PortConnection * conn =
          (PortConnection *) self->object;
        return conn->multiplier;
      }
      break;
    }

  g_return_val_if_reached (0.f);
}

/**
 * MAcro to get real value from knob value.
 */
#define REAL_VAL_FROM_KNOB(knob) \
  (self->min + (float) knob * (self->max - self->min))

/**
 * Converts from real value to knob value
 */
#define KNOB_VAL_FROM_REAL(real) \
  (((float) real - self->min) / (self->max - self->min))

/**
 * Sets real val
 */
static void
set_real_val (KnobWidget * self, float real_val)
{
  if (self->type == KNOB_TYPE_NORMAL)
    {
      (*self->setter) (self->object, real_val);
    }
  else
    {
      PortConnection * conn = (PortConnection *) self->object;
      conn->multiplier = real_val;
    }
}

/**
 * Draws the knob.
 */
static void
knob_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  KnobWidget * self = Z_KNOB_WIDGET (widget);

  int width = gtk_widget_get_allocated_width (widget);
  int height = gtk_widget_get_allocated_height (widget);

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  gtk_snapshot_render_background (
    snapshot, context, 0, 0, width, height);

  cairo_t * cr = gtk_snapshot_append_cairo (
    snapshot, &GRAPHENE_RECT_INIT (0, 0, width, height));

  cairo_pattern_t * shade_pattern;

  const float scale = (float) MIN (width, height);
  /* if the knob is 80 pixels wide, we want a
   * 3px line on it */
  const float pointer_thickness = 3.f * (scale / 80.f);

  const float start_angle =
    ((180.f - ARC_CUT_ANGLE) * (float) G_PI) / 180.f;
  const float end_angle =
    ((360.f + ARC_CUT_ANGLE) * (float) G_PI) / 180.f;

  self->last_real_val = get_real_val (self, true);
  const float value = KNOB_VAL_FROM_REAL (self->last_real_val);
  const float value_angle =
    start_angle + value * (end_angle - start_angle);
  float zero_angle =
    start_angle
    + ((self->zero - self->min) * (end_angle - start_angle));

  float value_x = cosf (value_angle);
  float value_y = sinf (value_angle);

  float xc = 0.5f + (float) width / 2.0f;
  float yc = 0.5f + (float) height / 2.0f;

  /* after this, everything is based on the center
   * of the knob */
  cairo_translate (cr, (double) xc, (double) yc);

  /* get the knob color from the theme */
  float center_radius = 0.48f * scale;
  float border_width = 0.8f;

  if (self->arc)
    {
      center_radius = scale * 0.33f;

      float inner_progress_radius = scale * 0.38f;
      float outer_progress_radius = scale * 0.48f;
      float progress_width =
        (outer_progress_radius - inner_progress_radius);
      float progress_radius =
        inner_progress_radius + progress_width / 2.f;

      /* dark surrounding arc background */
      cairo_set_source_rgb (cr, 0.3, 0.3, 0.3);
      cairo_set_line_width (cr, progress_width);
      cairo_arc (
        cr, 0, 0, progress_radius, start_angle, end_angle);
      cairo_stroke (cr);

      //look up the surrounding arc colors from the config
      // TODO

      //vary the arc color over the travel of the knob
      double intensity =
        fabs ((double) value - (double) self->zero)
        / MAX (
          (double) self->zero, (double) (1.f - self->zero));
      const double intensity_inv = 1.0 - intensity;
      double       r =
        intensity_inv * self->end_color.red
        + intensity * self->start_color.red;
      double g =
        intensity_inv * self->end_color.green
        + intensity * self->start_color.green;
      double b =
        intensity_inv * self->end_color.blue
        + intensity * self->start_color.blue;

      //draw the arc
      cairo_set_source_rgb (cr, r, g, b);
      cairo_set_line_width (cr, progress_width);
      if (zero_angle > value_angle)
        {
          cairo_arc (
            cr, 0, 0, progress_radius, value_angle,
            zero_angle);
        }
      else
        {
          cairo_arc (
            cr, 0, 0, progress_radius, zero_angle,
            value_angle);
        }
      cairo_stroke (cr);

      //shade the arc
      if (!self->flat)
        {
          /* note we have to offset the pattern from
           * our centerpoint */
          shade_pattern =
            cairo_pattern_create_linear (0.0, -yc, 0.0, yc);
          cairo_pattern_add_color_stop_rgba (
            shade_pattern, 0.0, 1, 1, 1, 0.15);
          cairo_pattern_add_color_stop_rgba (
            shade_pattern, 0.5, 1, 1, 1, 0.0);
          cairo_pattern_add_color_stop_rgba (
            shade_pattern, 1.0, 1, 1, 1, 0.0);
          cairo_set_source (cr, shade_pattern);
          cairo_arc (
            cr, 0, 0, (double) outer_progress_radius - 1.0, 0,
            2.0 * G_PI);
          cairo_fill (cr);
          cairo_pattern_destroy (shade_pattern);
        }

#if 0 //black border
      const float start_angle_x = cos (start_angle);
      const float start_angle_y = sin (start_angle);
      const float end_angle_x = cos (end_angle);
      const float end_angle_y = sin (end_angle);

      cairo_set_source_rgb (cr, 0, 0, 0 );
      cairo_set_line_width (cr, border_width);
      cairo_move_to (cr, (outer_progress_radius * start_angle_x), (outer_progress_radius * start_angle_y));
      cairo_line_to (cr, (inner_progress_radius * start_angle_x), (inner_progress_radius * start_angle_y));
      cairo_stroke (cr);
      cairo_move_to (cr, (outer_progress_radius * end_angle_x), (outer_progress_radius * end_angle_y));
      cairo_line_to (cr, (inner_progress_radius * end_angle_x), (inner_progress_radius * end_angle_y));
      cairo_stroke (cr);
      cairo_arc (cr, 0, 0, outer_progress_radius, start_angle, end_angle);
      cairo_stroke (cr);
#endif
    }

  if (!self->flat)
    {
      //knob shadow
      cairo_save (cr);
      cairo_translate (
        cr, pointer_thickness + 1, pointer_thickness + 1);
      cairo_set_source_rgba (cr, 0, 0, 0, 0.1);
      cairo_arc (cr, 0, 0, center_radius - 1, 0, 2.0 * G_PI);
      cairo_fill (cr);
      cairo_restore (cr);

      //inner circle
#define KNOB_COLOR 0, 90, 0, 0
      cairo_set_source_rgba (cr, KNOB_COLOR); /* knob color */
      cairo_arc (cr, 0, 0, center_radius, 0, 2.0 * G_PI);
      cairo_fill (cr);

      //gradient
      if (self->bevel)
        {
          //knob gradient
          shade_pattern = cairo_pattern_create_linear (
            0.0, -yc, 0.0,
            yc); //note we have to offset the gradient from our centerpoint
          cairo_pattern_add_color_stop_rgba (
            shade_pattern, 0.0, 1, 1, 1, 0.2);
          cairo_pattern_add_color_stop_rgba (
            shade_pattern, 0.2, 1, 1, 1, 0.2);
          cairo_pattern_add_color_stop_rgba (
            shade_pattern, 0.8, 0, 0, 0, 0.2);
          cairo_pattern_add_color_stop_rgba (
            shade_pattern, 1.0, 0, 0, 0, 0.2);
          cairo_set_source (cr, shade_pattern);
          cairo_arc (cr, 0, 0, center_radius, 0, 2.0 * G_PI);
          cairo_fill (cr);
          cairo_pattern_destroy (shade_pattern);

          //flat top over beveled edge
          cairo_set_source_rgba (cr, 90, 0, 0, 0.5);
          cairo_arc (
            cr, 0, 0, center_radius - pointer_thickness, 0,
            2.0 * G_PI);
          cairo_fill (cr);
        }
      else
        {
          /* radial gradient
           * note we have to offset the gradient
           * from our centerpoint */
          shade_pattern = cairo_pattern_create_radial (
            -center_radius, -center_radius, 1, -center_radius,
            -center_radius, center_radius * 2.5f);
          cairo_pattern_add_color_stop_rgba (
            shade_pattern, 0.0, 1, 1, 1, 0.2);
          cairo_pattern_add_color_stop_rgba (
            shade_pattern, 1.0, 0, 0, 0, 0.3);
          cairo_set_source (cr, shade_pattern);
          cairo_arc (cr, 0, 0, center_radius, 0, 2.0 * G_PI);
          cairo_fill (cr);
          cairo_pattern_destroy (shade_pattern);
        }
    }
  else
    {
      /* color inner circle */
      cairo_set_source_rgba (cr, 70, 70, 70, 0.2);
      cairo_arc (cr, 0, 0, center_radius, 0, 2.0 * G_PI);
      cairo_fill (cr);
    }

  //black knob border
  cairo_set_line_width (cr, border_width);
  cairo_set_source_rgba (cr, 0, 0, 0, 1);
  cairo_arc (cr, 0, 0, center_radius, 0, 2.0 * G_PI);
  cairo_stroke (cr);

  //line shadow
  if (!self->flat)
    {
      cairo_save (cr);
      cairo_translate (cr, 1, 1);
      cairo_set_source_rgba (cr, 0, 0, 0, 0.3);
      cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_width (cr, pointer_thickness);
      cairo_move_to (
        cr, (center_radius * value_x),
        (center_radius * value_y));
      cairo_line_to (
        cr, ((center_radius * 0.4f) * value_x),
        ((center_radius * 0.4f) * value_y));
      cairo_stroke (cr);
      cairo_restore (cr);
    }

  //line
  cairo_set_source_rgba (cr, 1, 1, 1, 1);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, pointer_thickness);
  cairo_move_to (
    cr, (center_radius * value_x), (center_radius * value_y));
  cairo_line_to (
    cr, ((center_radius * 0.4f) * value_x),
    ((center_radius * 0.4f) * value_y));
  cairo_stroke (cr);

  if (self->hover)
    {
      /* highlight */
      cairo_set_source_rgba (cr, 1, 1, 1, 0.12);
      cairo_arc (cr, 0, 0, center_radius, 0, 2.0 * G_PI);
      cairo_fill (cr);

      if (self->hover_str_getter)
        {
          /* draw text */
          char str[50];
          self->hover_str_getter (self->object, str);
          cairo_set_source_rgba (cr, 1, 1, 1, 1);
          int we, he;
          z_cairo_get_text_extents_for_widget (
            widget, self->layout, str, &we, &he);
          cairo_move_to (cr, -we / 2, -he / 2);
          z_cairo_draw_text_full (
            cr, widget, self->layout, str, width / 2 - we / 2,
            height / 2 - he / 2);
        }
    }

  cairo_identity_matrix (cr);

  cairo_destroy (cr);
}

static void
on_enter (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  gpointer                   user_data)
{
  KnobWidget * self = Z_KNOB_WIDGET (user_data);
  self->hover = true;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
on_leave (
  GtkEventControllerMotion * motion_controller,
  gpointer                   user_data)
{
  KnobWidget * self = Z_KNOB_WIDGET (user_data);
  if (!gtk_gesture_drag_get_offset (self->drag, NULL, NULL))
    self->hover = false;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
drag_update (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  KnobWidget *     self)
{
  offset_y = -offset_y;
  const int use_y =
    fabs (offset_y - self->last_y)
    > fabs (offset_x - self->last_x);
  const float cur_real_val = get_real_val (self, false);
  const float cur_knob_val = KNOB_VAL_FROM_REAL (cur_real_val);
  const float delta =
    use_y ? (float) (offset_y - self->last_y)
          : (float) (offset_x - self->last_x);
  const float multiplier = 0.004f;
  const float new_knob_val =
    CLAMP (cur_knob_val + multiplier * delta, 0.f, 1.f);
  const float new_real_val = REAL_VAL_FROM_KNOB (new_knob_val);
  set_real_val (self, new_real_val);
  self->last_x = offset_x;
  self->last_y = offset_y;

  gtk_widget_queue_draw (GTK_WIDGET (self));

  self->drag_updated = true;
}

static void
drag_end (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  KnobWidget *     self)
{
  GdkModifierType state =
    gtk_event_controller_get_current_event_state (
      GTK_EVENT_CONTROLLER (gesture));

  /* reset if ctrl-clicked */
  if (
    self->default_getter && self->setter
    && !self->drag_updated && state & GDK_CONTROL_MASK)
    {
      float def_fader_val =
        self->default_getter (self->object);
      self->setter (self->object, def_fader_val);
    }

  self->last_x = 0;
  self->last_y = 0;
  self->drag_updated = false;
}

static bool
tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  KnobWidget *    self)
{
  float real_val = get_real_val (self, true);
  if (!math_floats_equal (real_val, self->last_real_val))
    {
      gtk_widget_queue_draw (widget);
    }

  return G_SOURCE_CONTINUE;
}

/**
 * Creates a knob widget with the given options and
 * binds it to the given value.
 *
 * @param get_val Getter function.
 * @param get_default_val Getter function for
 *   default value.
 * @param set_val Setter function.
 * @param object Object to call get/set with.
 * @param dest Port destination multiplier index, if
 *   type is Port, otherwise ignored.
 */
KnobWidget *
_knob_widget_new (
  GenericFloatGetter get_val,
  GenericFloatGetter get_default_val,
  GenericFloatSetter set_val,
  void *             object,
  KnobType           type,
  float              min,
  float              max,
  int                size,
  float              zero)
{
  g_warn_if_fail (object);

  KnobWidget * self = g_object_new (KNOB_WIDGET_TYPE, NULL);
  /*self->value = value;*/
  self->getter = get_val;
  self->default_getter = get_default_val;
  self->setter = set_val;
  self->object = object;
  self->min = min;
  self->max = max;
  self->type = type;
  /*self->cur = get_knob_val (self)*/
  self->size = size; /* default 30 */
  self->hover = 0;
  self->zero = zero; /* default 0.05f */
  self->arc = 1;
  self->bevel = 1;
  self->flat = 1;
  gdk_rgba_parse (&self->start_color, "rgb(78%,78%,78%)");
  gdk_rgba_parse (&self->end_color, "rgb(66%,66%,66%)");
  self->last_x = 0;
  self->last_y = 0;

  /* set size */
  gtk_widget_set_size_request (GTK_WIDGET (self), size, size);

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (
      gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion_controller), "enter",
    G_CALLBACK (on_enter), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave",
    G_CALLBACK (on_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (motion_controller));

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update",
    G_CALLBACK (drag_update), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end", G_CALLBACK (drag_end),
    self);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) tick_cb, self, NULL);

  return self;
}

static void
dispose (KnobWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  G_OBJECT_CLASS (knob_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
finalize (KnobWidget * self)
{
  if (self->layout)
    g_object_unref (self->layout);

  G_OBJECT_CLASS (knob_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
knob_widget_init (KnobWidget * self)
{
  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
  gtk_widget_set_parent (
    GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag));

  self->layout = z_cairo_create_pango_layout_from_string (
    (GtkWidget *) self, "7", PANGO_ELLIPSIZE_NONE, -1);
}

static void
knob_widget_class_init (KnobWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = knob_snapshot;

  gtk_widget_class_set_layout_manager_type (
    wklass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}
