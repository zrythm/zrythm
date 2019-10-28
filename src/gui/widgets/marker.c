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
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/marker_dialog.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/ui.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  MarkerWidget,
  marker_widget,
  ARRANGER_OBJECT_WIDGET_TYPE)

static gboolean
marker_draw_cb (
  GtkWidget * widget,
  cairo_t *   cr,
  MarkerWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);
  if (ao_prv->redraw)
    {
      GtkStyleContext * context =
        gtk_widget_get_style_context (widget);

      int width =
        gtk_widget_get_allocated_width (widget);
      int height =
        gtk_widget_get_allocated_height (widget);

      ao_prv->cached_surface =
        cairo_surface_create_similar (
          cairo_get_target (cr),
          CAIRO_CONTENT_COLOR_ALPHA,
          width, height);
      ao_prv->cached_cr =
        cairo_create (ao_prv->cached_surface);

      gtk_render_background (
        context, ao_prv->cached_cr,
        0, 0, width, height);

      /* set color */
      GdkRGBA color = P_MARKER_TRACK->color;
      ui_get_arranger_object_color (
        &color,
        gtk_widget_get_state_flags (
          GTK_WIDGET (self)) &
          GTK_STATE_FLAG_PRELIGHT,
        marker_is_selected (self->marker),
        marker_is_transient (self->marker));
      gdk_cairo_set_source_rgba (
        ao_prv->cached_cr, &color);

      cairo_rectangle (
        ao_prv->cached_cr, 0, 0,
        width - MARKER_WIDGET_TRIANGLE_W, height);
      cairo_fill (ao_prv->cached_cr);

      cairo_move_to (
        ao_prv->cached_cr,
        width - MARKER_WIDGET_TRIANGLE_W, 0);
      cairo_line_to (
        ao_prv->cached_cr, width, height);
      cairo_line_to (
        ao_prv->cached_cr,
        width - MARKER_WIDGET_TRIANGLE_W, height);
      cairo_line_to (
        ao_prv->cached_cr,
        width - MARKER_WIDGET_TRIANGLE_W, 0);
      cairo_close_path (ao_prv->cached_cr);
      cairo_fill (ao_prv->cached_cr);

      char str[100];
      sprintf (str, "%s", self->marker->name);
      if (DEBUGGING &&
          marker_is_transient (self->marker))
        {
          strcat (str, " [t]");
        }

      GdkRGBA c2;
      ui_get_contrast_color (
        &color, &c2);
      gdk_cairo_set_source_rgba (
        ao_prv->cached_cr, &c2);
      z_cairo_draw_text (
        ao_prv->cached_cr, widget,
        self->layout, str);

      ao_prv->redraw = 0;
    }

  cairo_set_source_surface (
    cr, ao_prv->cached_surface, 0, 0);
  cairo_paint (cr);

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

MarkerWidget *
marker_widget_new (Marker * marker)
{
  MarkerWidget * self =
    g_object_new (MARKER_WIDGET_TYPE,
                  "visible", 1,
                  NULL);

  arranger_object_widget_setup (
    Z_ARRANGER_OBJECT_WIDGET (self),
    (ArrangerObject *) marker);

  self->marker = marker;

  marker_widget_recreate_pango_layouts (self);

  return self;
}

void
marker_widget_recreate_pango_layouts (
  MarkerWidget * self)
{
  if (PANGO_IS_LAYOUT (self->layout))
    g_object_unref (self->layout);

  g_return_if_fail (
    self->marker && self->marker->name);

  self->layout =
    z_cairo_create_default_pango_layout (
      (GtkWidget *) self);
  z_cairo_get_text_extents_for_widget (
    (GtkWidget *) self, self->layout,
    self->marker->name, &self->textw, &self->texth);
}


static void
on_screen_changed (
  GtkWidget *          widget,
  GdkScreen *          previous_screen,
  MarkerWidget * self)
{
  marker_widget_recreate_pango_layouts (self);
}

static void
finalize (
  MarkerWidget * self)
{
  if (self->layout)
    g_object_unref (self->layout);
  if (self->mp)
    g_object_unref (self->mp);

  G_OBJECT_CLASS (
    marker_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
marker_widget_class_init (MarkerWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);

  GObjectClass * oklass =
    G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) finalize;
}

static void
marker_widget_init (MarkerWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  self->mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (ao_prv->drawing_area)));

  /* connect signals */
  g_signal_connect (
    G_OBJECT (ao_prv->drawing_area), "draw",
    G_CALLBACK (marker_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self->mp), "pressed",
    G_CALLBACK (on_press), self);
  g_signal_connect (
    G_OBJECT (self), "screen-changed",
    G_CALLBACK (on_screen_changed),  self);
}
