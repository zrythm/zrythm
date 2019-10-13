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

#include "audio/marker.h"
#include "audio/marker_track.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/marker_dialog.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/ui.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  MarkerWidget,
  marker_widget,
  GTK_TYPE_BOX)

static gboolean
marker_draw_cb (
  GtkWidget * widget,
  cairo_t *   cr,
  MarkerWidget * self)
{
  GtkStyleContext *context;

  context =
    gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  GdkRGBA * color = &P_MARKER_TRACK->color;
  cairo_set_source_rgba (
    cr, color->red, color->green, color->blue,
    marker_is_transient (self->marker) ?
      0.7 : 1);
  if (marker_is_selected (self->marker))
    {
      cairo_set_source_rgba (
        cr, color->red + 0.4, color->green + 0.2,
        color->blue + 0.2, 1);
    }
  cairo_rectangle (
    cr, 0, 0,
    width - MARKER_WIDGET_TRIANGLE_W, height);
  cairo_fill(cr);

  cairo_move_to (
    cr, width - MARKER_WIDGET_TRIANGLE_W, 0);
  cairo_line_to (
    cr, width, height);
  cairo_line_to (
    cr, width - MARKER_WIDGET_TRIANGLE_W, height);
  cairo_line_to (
    cr, width - MARKER_WIDGET_TRIANGLE_W, 0);
  cairo_close_path (cr);
  cairo_fill(cr);

  char str[100];
  sprintf (str, "%s", self->marker->name);
  if (DEBUGGING &&
      marker_is_transient (self->marker))
    {
      strcat (str, " [t]");
    }

  GdkRGBA c2;
  ui_get_contrast_text_color (
    color, &c2);
  cairo_set_source_rgba (
    cr, c2.red, c2.green, c2.blue, 1.0);
  PangoLayout * layout =
    z_cairo_create_default_pango_layout (
      widget);
  z_cairo_draw_text (
    cr, widget, layout, str);
  g_object_unref (layout);

 return FALSE;
}

static void
on_press (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  MarkerWidget *   self)
{
  if (n_press == 2)
    {
      MarkerDialogWidget * dialog =
        marker_dialog_widget_new (self);
      gtk_widget_show_all (GTK_WIDGET (dialog));
    }
}

/**
 * Sets hover in CSS.
 */
static int
on_motion (
  GtkWidget * widget,
  GdkEventMotion *event,
  MarkerWidget * self)
{
  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT,
        0);
    }

  return FALSE;
}

MarkerWidget *
marker_widget_new (Marker * marker)
{
  MarkerWidget * self =
    g_object_new (MARKER_WIDGET_TYPE,
                  "visible", 1,
                  NULL);

  self->marker = marker;

  return self;
}

static void
marker_widget_class_init (MarkerWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "marker");
}

static void
marker_widget_init (MarkerWidget * self)
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

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self->drawing_area), "draw",
    G_CALLBACK (marker_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self->drawing_area),
    "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (self->drawing_area),
    "motion-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (self->drawing_area),
    "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (self->mp), "pressed",
    G_CALLBACK (on_press), self);

  g_object_ref (self);
}
