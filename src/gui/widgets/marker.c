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

/**
 * Recreates the pango layouts for drawing.
 *
 * @param width Full width of the marker.
 */
void
marker_recreate_pango_layouts (
  Marker * self)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  if (!PANGO_IS_LAYOUT (self->layout))
    {
      PangoFontDescription *desc;
      self->layout =
        gtk_widget_create_pango_layout (
          GTK_WIDGET (
            arranger_object_get_arranger (obj)),
          NULL);
      desc =
        pango_font_description_from_string (
          MARKER_NAME_FONT);
      pango_layout_set_font_description (
        self->layout, desc);
      pango_font_description_free (desc);
    }
  pango_layout_get_pixel_size (
    self->layout, &obj->textw, &obj->texth);
}

/**
 *
 * @param cr Cairo context of the arranger.
 * @param rect Rectangle in the arranger.
 */
void
marker_draw (
  Marker *       self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  ArrangerWidget * arranger =
    arranger_object_get_arranger (obj);

  /* set color */
  GdkRGBA color = P_MARKER_TRACK->color;
  ui_get_arranger_object_color (
    &color,
    arranger->hovered_object == obj,
    marker_is_selected (self), 0);
  gdk_cairo_set_source_rgba (
    cr, &color);

  cairo_rectangle (
    cr, obj->full_rect.x - rect->x,
    obj->full_rect.y - rect->y,
    obj->full_rect.width -
      MARKER_WIDGET_TRIANGLE_W,
    obj->full_rect.height);
  cairo_fill (cr);

  cairo_move_to (
    cr,
    (obj->full_rect.x + obj->full_rect.width) -
      (MARKER_WIDGET_TRIANGLE_W + rect->x),
    obj->full_rect.y - rect->y);
  cairo_line_to (
    cr,
    (obj->full_rect.x + obj->full_rect.width) -
      rect->x,
    (obj->full_rect.y + obj->full_rect.height) -
      rect->y);
  cairo_line_to (
    cr,
    (obj->full_rect.x + obj->full_rect.width) -
      (MARKER_WIDGET_TRIANGLE_W + rect->x),
    (obj->full_rect.y + obj->full_rect.height) -
      rect->y);
  cairo_line_to (
    cr,
    (obj->full_rect.x + obj->full_rect.width) -
      (MARKER_WIDGET_TRIANGLE_W + rect->x),
    obj->full_rect.y - rect->y);
  cairo_close_path (cr);
  cairo_fill (cr);

  char str[100];
  sprintf (str, "%s", self->name);

  GdkRGBA c2;
  ui_get_contrast_color (&color, &c2);
  gdk_cairo_set_source_rgba (cr, &c2);
  cairo_move_to (
    cr,
    (obj->full_rect.x + MARKER_NAME_PADDING) -
      rect->x,
    (obj->full_rect.y + MARKER_NAME_PADDING) -
      rect->y);
  z_cairo_draw_text (
    cr, GTK_WIDGET (arranger), self->layout, str);
}

#if 0
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
#endif
