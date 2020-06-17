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
#include "zrythm_app.h"

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
    marker_is_selected (self), false, false);
  gdk_cairo_set_source_rgba (
    cr, &color);

  z_cairo_rounded_rectangle (
    cr,
    obj->full_rect.x - rect->x,
    obj->full_rect.y - rect->y,
    obj->full_rect.width,
    obj->full_rect.height, 1, 4);
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
