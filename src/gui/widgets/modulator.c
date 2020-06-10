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

#include "audio/modulator.h"
#include "audio/track.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/knob_with_name.h"
#include "gui/widgets/modulator.h"
#include "utils/arrays.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (ModulatorWidget,
               modulator_widget,
               TWO_COL_EXPANDER_BOX_WIDGET_TYPE)

static gboolean
modulator_graph_draw (
  GtkWidget    *widget,
  cairo_t *cr,
  ModulatorWidget * self)
{
  GtkStyleContext *context =
    gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);
  cairo_set_source_rgba (
    cr, 0, 0, 0, 1);
  cairo_rectangle (
    cr, 0, 0, width, height);
  cairo_set_source_rgba (
    cr, 0, 1, 0.5, 1);

  Plugin * pl =
    self->modulator->plugin;
  /*int num_cv_outs =*/
    /*pl->descr->num_cv_outs;*/
  Port * port;
  float maxf, minf, sizef, normalized_val;
  int i, j;
  for (i = 0; i < pl->num_out_ports; i++)
    {
      port = pl->out_ports[i];

      if (port->id.type != TYPE_CV ||
          port->id.flow != FLOW_OUTPUT)
        continue;

      maxf = 1.f;
      minf = -1.f;
      sizef = maxf - minf;

      normalized_val =
        (port->buf[0] - minf) / sizef;

      self->prev_points[i][59] =
        (double) normalized_val *
        (double) height;

      /* draw prev points */
      for (j = 0; j < 59; j++)
        {
          cairo_move_to (
            cr, j,
            self->prev_points[i][j]);
          cairo_line_to (
            cr, j,
            self->prev_points[i][j + 1]);
          cairo_stroke (cr);

          /* replace already processed point */
          self->prev_points[i][j] =
            self->prev_points[i][j + 1];
        }
      if (i == 0)
        break;
    }

  return FALSE;
}

static gboolean
graph_tick_callback (
  GtkWidget *widget,
  GdkFrameClock *frame_clock,
  ModulatorWidget * self)
{
  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

/** Getter for the KnobWidget. */
static float
get_control_value (
  Port * port)
{
  return port_get_control_value (port, 0);
}

/** Setter for the KnobWidget. */
static void
set_control_value (
  Port * port,
  float  value)
{
  port_set_control_value (
    port, value, 0, 0);
}

ModulatorWidget *
modulator_widget_new (
  Modulator * modulator)
{
  ModulatorWidget * self =
    g_object_new (MODULATOR_WIDGET_TYPE, NULL);

  self->modulator = modulator;
  g_warn_if_fail (modulator);

  expander_box_widget_set_label (
    Z_EXPANDER_BOX_WIDGET (self),
    modulator->name);
  expander_box_widget_set_icon_name (
    Z_EXPANDER_BOX_WIDGET (self),
    "insert-math-expression");

  self->controls_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 2));
  gtk_widget_set_visible (
    GTK_WIDGET (self->controls_box), 1);

  Port * port;
  for (int i = 0;
       i < modulator->plugin->num_in_ports; i++)
    {
      port = modulator->plugin->in_ports[i];

      if (port->id.type != TYPE_CONTROL ||
          port->id.flow != FLOW_INPUT)
        continue;


      KnobWidget * knob =
        knob_widget_new_simple (
          get_control_value,
          set_control_value,
          port,
          port->lv2_port->lv2_control->minf,
          port->lv2_port->lv2_control->maxf,
          24,
          port->lv2_port->lv2_control->minf);
      KnobWithNameWidget * knob_with_name =
        knob_with_name_widget_new (
          port->id.label,
          knob);

      array_append (self->knobs,
                    self->num_knobs,
                    knob_with_name);

      gtk_container_add (
        GTK_CONTAINER (self->controls_box),
        GTK_WIDGET (knob_with_name));
    }

  self->graph =
    GTK_DRAWING_AREA (
      gtk_drawing_area_new ());
  g_signal_connect (
    G_OBJECT (self->graph), "draw",
    G_CALLBACK (modulator_graph_draw), self);
  gtk_widget_set_size_request (
    GTK_WIDGET (self->graph),
    60, -1);
  gtk_widget_set_visible (
    GTK_WIDGET (self->graph), 1);
  gtk_widget_add_tick_callback (
    GTK_WIDGET (self->graph),
    (GtkTickCallback) graph_tick_callback,
    self,
    NULL);

  two_col_expander_box_widget_add_pair (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->controls_box),
    GTK_WIDGET (self->graph));

  g_object_ref (self);

  return self;
}

static void
modulator_widget_class_init (
  ModulatorWidgetClass * _klass)
{
  /*GtkWidgetClass * klass =*/
    /*GTK_WIDGET_CLASS (_klass);*/
}

static void
modulator_widget_init (
  ModulatorWidget * self)
{
  expander_box_widget_set_orientation (
    Z_EXPANDER_BOX_WIDGET (self),
    GTK_ORIENTATION_HORIZONTAL);
}
