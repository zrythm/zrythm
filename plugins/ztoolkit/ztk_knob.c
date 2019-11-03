/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 * Copyright (C) 2010 Paul Davis
 *
 * This file is part of ZPlugins
 *
 * ZPlugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ZPlugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "ztoolkit/ztk_app.h"
#include "ztoolkit/ztk_knob.h"

#define ARC_CUT_ANGLE 60.f

/**
 * Macro to get real value.
 */
#define GET_REAL_VAL \
  ((*self->getter) (self->object))

/**
 * MAcro to get real value from knob value.
 */
#define REAL_VAL_FROM_KNOB(knob) \
  (self->min + (float) knob * \
   (self->max - self->min))

/**
 * Converts from real value to knob value
 */
#define KNOB_VAL_FROM_REAL(real) \
  (((float) real - self->min) / \
   (self->max - self->min))

/**
 * Sets real val
 */
#define SET_REAL_VAL(real) \
   ((*self->setter)(self->object, (float) real))

#define MAX(x,y) (x > y ? x : y)
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

static void
ztk_knob_draw_cb (
  ZtkWidget * widget,
  cairo_t * cr)
{
  ZtkKnob * self = (ZtkKnob *) widget;
  PuglRect * rect = &widget->rect;
  int width = (int) rect->width;
  int height = (int) rect->height;

  const float scale = (float) width;
  /* if the knob is 80 pixels wide, we want a
   * 3px line on it */
  const float pointer_thickness =
    3.f * (scale / 80.f);

  const float start_angle =
    ((180.f - ARC_CUT_ANGLE) * (float) M_PI) / 180.f;
  const float end_angle =
    ((360.f + ARC_CUT_ANGLE) * (float) M_PI) / 180.f;

  const float value =
    KNOB_VAL_FROM_REAL (GET_REAL_VAL);
  const float value_angle = start_angle
    + value * (end_angle - start_angle);
  const float zero_angle =
    start_angle +
    (self->zero * (end_angle - start_angle));

  float value_x = cosf (value_angle);
  float value_y = sinf (value_angle);

  float xc =  0.5f + (float) width / 2.0f;
  float yc = 0.5f + (float) height / 2.0f;

  /* after this, everything is based on the center
   * of the knob */
	cairo_save(cr);
  cairo_translate (
    cr, (double) rect->x + (double) xc,
    (double) rect->y + (double) yc);

  /* get the knob color from the theme */
  float center_radius = 0.48f * scale;
  float border_width = 0.8f;

  center_radius = scale * 0.33f;

  float inner_progress_radius =
    scale * 0.38f;
  float outer_progress_radius =
    scale * 0.48f;
  float progress_width =
    (outer_progress_radius -
     inner_progress_radius);
  float progress_radius =
    inner_progress_radius + progress_width / 2.f;

  /* dark surrounding arc background */
  cairo_set_source_rgb (
    cr, 0.3, 0.3, 0.3 );
  cairo_set_line_width (
    cr, progress_width);
  cairo_arc (
    cr, 0, 0, progress_radius, start_angle,
    end_angle);
  cairo_stroke (cr);

  //vary the arc color over the travel of the knob
  double intensity =
    fabs ((double) value - (double) self->zero) /
    MAX (
      (double) self->zero,
      (double) (1.f - self->zero));
  const double intensity_inv =
    1.0 - intensity;
  double r =
    intensity_inv * self->end_color.red   +
    intensity * self->start_color.red;
  double g =
    intensity_inv * self->end_color.green +
    intensity * self->start_color.green;
  double b =
    intensity_inv * self->end_color.blue  +
    intensity * self->start_color.blue;

  //draw the arc
  cairo_set_source_rgb (
    cr, r, g, b);
  cairo_set_line_width (cr, progress_width);
  if (zero_angle > value_angle)
    {
      cairo_arc (
        cr, 0, 0, progress_radius,
        value_angle, zero_angle);
    }
  else
    {
      cairo_arc (
        cr, 0, 0, progress_radius,
        zero_angle, value_angle);
    }
  cairo_stroke (cr);

  /* color inner circle */
  cairo_set_source_rgba (
    cr, 70, 70, 70, 0.2);
  cairo_arc (
    cr, 0, 0, center_radius, 0, 2.0 * M_PI);
  cairo_fill (cr);


  //black knob border
  cairo_set_line_width (cr, border_width);
  cairo_set_source_rgba (cr, 0,0,0, 1 );
  cairo_arc (cr, 0, 0, center_radius, 0, 2.0*M_PI);
  cairo_stroke (cr);

  //line
  cairo_set_source_rgba (cr, 1,1,1, 1 );
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, pointer_thickness);
  cairo_move_to (
    cr, (center_radius * value_x),
    (center_radius * value_y));
  cairo_line_to (
    cr, ((center_radius*0.4f) * value_x),
    ((center_radius*0.4f) * value_y));
  cairo_stroke (cr);

  //highlight if grabbed or if mouse is hovering over
  //me
  if (widget->state &
      ZTK_WIDGET_STATE_SELECTED)
    {
      cairo_set_source_rgba (cr, 1,1,1, 0.24 );
      cairo_arc (
        cr, 0, 0, center_radius, 0, 2.0*M_PI);
      cairo_fill (cr);
    }
  else if (widget->state &
      ZTK_WIDGET_STATE_HOVERED)
    {
      cairo_set_source_rgba (cr, 1,1,1, 0.12 );
      cairo_arc (
        cr, 0, 0, center_radius, 0, 2.0*M_PI);
      cairo_fill (cr);
    }

	cairo_restore(cr);
}

static void
ztk_knob_update_cb (
  ZtkWidget * w)
{
  ZtkKnob * self = (ZtkKnob *) w;
  ZtkApp * app = w->app;
  if (w->state & ZTK_WIDGET_STATE_PRESSED)
    {
      double delta =
        app->prev_press_y -
        app->offset_press_y;

      SET_REAL_VAL (
        REAL_VAL_FROM_KNOB (
          CLAMP (
            KNOB_VAL_FROM_REAL (GET_REAL_VAL) +
              0.004f * (float) delta,
             0.0f, 1.0f)));
    }
}

/**
 * Creates a new knob.
 *
 * @param get_val Getter function.
 * @param set_val Setter function.
 * @param object Object to call get/set with.
 * @param dest Port destination multiplier index, if
 *   type is Port, otherwise ignored.
 */
ZtkKnob *
ztk_knob_new (
  PuglRect * rect,
  float (*get_val)(void *),
  void (*set_val)(void *, float),
  void * object,
  float  min,
  float  max,
  float  zero)
{
  ZtkKnob * self = calloc (1, sizeof (ZtkKnob));
  ztk_widget_init (
    (ZtkWidget *) self, rect,
    ztk_knob_update_cb, ztk_knob_draw_cb,
    ztk_knob_free);

  self->getter = get_val;
  self->setter = set_val;
  self->object = object;
  self->min = min;
  self->max = max;
  self->start_color.red = 78.0 / 100.0;
  self->start_color.green = 78.0 / 100.0;
  self->start_color.blue = 78.0 / 100.0;
  self->end_color.red = 66.0 / 100.0;
  self->end_color.green = 66.0 / 100.0;
  self->end_color.blue = 66.0 / 100.0;

  return self;
}

void
ztk_knob_free (
  ZtkWidget * widget)
{
  ZtkKnob * self = (ZtkKnob *) widget;

  free (self);
}
