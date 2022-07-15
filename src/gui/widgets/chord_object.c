// SPDX-FileCopyrightText: © 2018-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/chord_selector_window.h"
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
chord_object_recreate_pango_layouts (ChordObject * self)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  if (!PANGO_IS_LAYOUT (self->layout))
    {
      PangoFontDescription * desc;
      self->layout = gtk_widget_create_pango_layout (
        GTK_WIDGET (arranger_object_get_arranger (obj)), NULL);
      desc = pango_font_description_from_string (
        CHORD_OBJECT_NAME_FONT);
      pango_layout_set_font_description (self->layout, desc);
      pango_font_description_free (desc);
    }
  pango_layout_get_pixel_size (
    self->layout, &obj->textw, &obj->texth);
}

/**
 * Draws the given chord object.
 */
void
chord_object_draw (ChordObject * self, GtkSnapshot * snapshot)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  ArrangerWidget * arranger =
    arranger_object_get_arranger (obj);

  /* get color */
  GdkRGBA color = P_CHORD_TRACK->color;
  ui_get_arranger_object_color (
    &color, arranger->hovered_object == obj,
    chord_object_is_selected (self), false, false);
  ChordDescriptor * descr =
    chord_object_get_chord_descriptor (self);

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

  char str[100];
  chord_descriptor_to_string (descr, str);
  char display_str[200];
  if (DEBUGGING)
    {
      sprintf (display_str, "%d %s", self->chord_index, str);
    }
  else
    {
      strcpy (display_str, str);
    }

  GdkRGBA c2;
  ui_get_contrast_color (&color, &c2);

  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (
    snapshot,
    &GRAPHENE_POINT_INIT (
      obj->full_rect.x + CHORD_OBJECT_NAME_PADDING,
      obj->full_rect.y + CHORD_OBJECT_NAME_PADDING));
  pango_layout_set_text (self->layout, display_str, -1);
  gtk_snapshot_append_layout (snapshot, self->layout, &c2);
  gtk_snapshot_restore (snapshot);

  /* pop clip */
  gtk_snapshot_pop (snapshot);
}
