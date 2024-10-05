// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/marker.h"
#include "dsp/marker_track.h"
#include "dsp/tracklist.h"
#include "gui/cpp/gtk_widgets/arranger.h"
#include "gui/cpp/gtk_widgets/marker.h"
#include "project.h"
#include "utils/gtk.h"
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
  if (!PANGO_IS_LAYOUT (self->layout_.get ()))
    {
      PangoFontDescription * desc;
      self->layout_ = PangoLayoutUniquePtr (gtk_widget_create_pango_layout (
        GTK_WIDGET (self->get_arranger ()), nullptr));
      desc = pango_font_description_from_string (MARKER_NAME_FONT);
      pango_layout_set_font_description (self->layout_.get (), desc);
      pango_font_description_free (desc);
    }
  pango_layout_get_pixel_size (
    self->layout_.get (), &self->textw_, &self->texth_);
}

/**
 * Draws the given marker.
 */
void
marker_draw (Marker * self, GtkSnapshot * snapshot)
{
  // ArrangerWidget * arranger = self->get_arranger ();

  /* set color */
  auto color = Color::get_arranger_object_color (
    P_MARKER_TRACK->color_, self->is_hovered (), self->is_selected (), false,
    false);

  /* create clip */
  GskRoundedRect  rounded_rect;
  graphene_rect_t graphene_rect = Z_GRAPHENE_RECT_INIT (
    (float) self->full_rect_.x, (float) self->full_rect_.y,
    (float) self->full_rect_.width, (float) self->full_rect_.height);
  gsk_rounded_rect_init_from_rect (
    &rounded_rect, &graphene_rect, (float) self->full_rect_.height / 6.0f);
  gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);

  /* fill */
  auto color_rgba = color.to_gdk_rgba ();
  gtk_snapshot_append_color (snapshot, &color_rgba, &graphene_rect);

  z_return_if_fail (!self->escaped_name_.empty ());

  GdkRGBA c2 = color.get_contrast_color ().to_gdk_rgba ();

  gtk_snapshot_save (snapshot);
  {
    graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
      (float) self->full_rect_.x + MARKER_NAME_PADDING,
      (float) self->full_rect_.y + MARKER_NAME_PADDING);
    gtk_snapshot_translate (snapshot, &tmp_pt);
  }
  pango_layout_set_text (self->layout_.get (), self->escaped_name_.c_str (), -1);
  gtk_snapshot_append_layout (snapshot, self->layout_.get (), &c2);
  gtk_snapshot_restore (snapshot);

  /* pop rounded rect */
  gtk_snapshot_pop (snapshot);
}
