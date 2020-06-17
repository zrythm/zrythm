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
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/scale_selector_window.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/ui.h"
#include "zrythm_app.h"

/**
 * Recreates the pango layouts for drawing.
 *
 * @param width Full width of the marker.
 */
void
scale_object_recreate_pango_layouts (
  ScaleObject * self)
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
          SCALE_OBJECT_NAME_FONT);
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
scale_object_draw (
  ScaleObject *  self,
  cairo_t *      cr,
  GdkRectangle * rect)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  ArrangerWidget * arranger =
    arranger_object_get_arranger (obj);

  /* set color */
  GdkRGBA color = P_CHORD_TRACK->color;
  ui_get_arranger_object_color (
    &color,
    arranger->hovered_object == obj,
    scale_object_is_selected (self), false, false);
  gdk_cairo_set_source_rgba (cr, &color);

  z_cairo_rounded_rectangle (
    cr,
    obj->full_rect.x - rect->x,
    obj->full_rect.y - rect->y,
    obj->full_rect.width,
    obj->full_rect.height, 1, 4);
  cairo_fill (cr);

  char * str =
    musical_scale_to_string (self->scale);

  GdkRGBA c2;
  ui_get_contrast_color (&color, &c2);
  gdk_cairo_set_source_rgba (cr, &c2);
  cairo_move_to (
    cr,
    (obj->full_rect.x + SCALE_OBJECT_NAME_PADDING) -
      rect->x,
    (obj->full_rect.y + SCALE_OBJECT_NAME_PADDING) -
      rect->y);
  z_cairo_draw_text (
    cr, GTK_WIDGET (arranger), self->layout, str);
  g_free (str);
}

#if 0
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

ScaleObjectWidget *
scale_object_widget_new (ScaleObject * scale)
{
  ScaleObjectWidget * self =
    g_object_new (SCALE_OBJECT_WIDGET_TYPE, NULL);

  arranger_object_widget_setup (
    Z_ARRANGER_OBJECT_WIDGET (self),
    (ArrangerObject *) scale);

  self->scale_object = scale;

  char scale_str[100];
  musical_scale_strcpy (
    scale->scale, scale_str);
  PangoLayout * layout =
    z_cairo_create_default_pango_layout (
      (GtkWidget *) self);
  z_cairo_get_text_extents_for_widget (
    (GtkWidget *) self, layout,
    scale_str, &self->textw, &self->texth);
  g_object_unref (layout);

  return self;
}

static void
scale_object_widget_init (
  ScaleObjectWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  self->mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (ao_prv->drawing_area)));

  g_signal_connect (
    G_OBJECT (ao_prv->drawing_area), "draw",
    G_CALLBACK (scale_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self->mp), "pressed",
    G_CALLBACK (on_press), self);
}
#endif
