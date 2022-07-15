/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
marker_recreate_pango_layouts (Marker * self)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  if (!PANGO_IS_LAYOUT (self->layout))
    {
      PangoFontDescription * desc;
      self->layout = gtk_widget_create_pango_layout (
        GTK_WIDGET (arranger_object_get_arranger (obj)), NULL);
      desc =
        pango_font_description_from_string (MARKER_NAME_FONT);
      pango_layout_set_font_description (self->layout, desc);
      pango_font_description_free (desc);
    }
  pango_layout_get_pixel_size (
    self->layout, &obj->textw, &obj->texth);
}

/**
 * Draws the given marker.
 */
void
marker_draw (Marker * self, GtkSnapshot * snapshot)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  ArrangerWidget * arranger =
    arranger_object_get_arranger (obj);

  /* set color */
  GdkRGBA color = P_MARKER_TRACK->color;
  ui_get_arranger_object_color (
    &color, arranger->hovered_object == obj,
    marker_is_selected (self), false, false);

  /* create clip */
  GskRoundedRect  rounded_rect;
  graphene_rect_t graphene_rect = GRAPHENE_RECT_INIT (
    obj->full_rect.x, obj->full_rect.y, obj->full_rect.width,
    obj->full_rect.height);
  gsk_rounded_rect_init_from_rect (
    &rounded_rect, &graphene_rect,
    obj->full_rect.height / 6.0f);
  gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);

  /* fill */
  gtk_snapshot_append_color (snapshot, &color, &graphene_rect);

  g_return_if_fail (self->escaped_name);

  GdkRGBA c2;
  ui_get_contrast_color (&color, &c2);

  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (
    snapshot,
    &GRAPHENE_POINT_INIT (
      obj->full_rect.x + MARKER_NAME_PADDING,
      obj->full_rect.y + MARKER_NAME_PADDING));
  pango_layout_set_text (self->layout, self->escaped_name, -1);
  gtk_snapshot_append_layout (snapshot, self->layout, &c2);
  gtk_snapshot_restore (snapshot);

  /* pop rounded rect */
  gtk_snapshot_pop (snapshot);
}
