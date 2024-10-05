// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_track.h"
#include "dsp/scale_object.h"
#include "dsp/tracklist.h"
#include "gui/cpp/gtk_widgets/arranger.h"
#include "gui/cpp/gtk_widgets/scale_object.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/ui.h"
#include "zrythm_app.h"

/**
 * Recreates the pango layouts for drawing.
 *
 * @param width Full width of the marker.
 */
void
scale_object_recreate_pango_layouts (ScaleObject * self)
{
  if (!PANGO_IS_LAYOUT (self->layout_.get ()))
    {
      PangoFontDescription * desc;
      self->layout_ = PangoLayoutUniquePtr (gtk_widget_create_pango_layout (
        GTK_WIDGET (self->get_arranger ()), nullptr));
      desc = pango_font_description_from_string (SCALE_OBJECT_NAME_FONT);
      pango_layout_set_font_description (self->layout_.get (), desc);
      pango_font_description_free (desc);
    }
  pango_layout_get_pixel_size (
    self->layout_.get (), &self->textw_, &self->texth_);
}

/**
 * Draws the given scale object.
 */
void
scale_object_draw (ScaleObject * self, GtkSnapshot * snapshot)
{
  // ArrangerWidget * arranger = self->get_arranger ();

  /* set color */
  auto color = Color::get_arranger_object_color (
    P_CHORD_TRACK->color_, self->is_hovered (), self->is_selected (), false,
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
  z_gtk_snapshot_append_color (snapshot, color, &graphene_rect);

  auto str = self->scale_.to_string ();

  auto c2 = color.get_contrast_color ();

  gtk_snapshot_save (snapshot);
  {
    graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
      (float) self->full_rect_.x + SCALE_OBJECT_NAME_PADDING,
      (float) self->full_rect_.y + SCALE_OBJECT_NAME_PADDING);
    gtk_snapshot_translate (snapshot, &tmp_pt);
  }
  pango_layout_set_text (self->layout_.get (), str.c_str (), -1);
  auto c2_rgba = c2.to_gdk_rgba ();
  gtk_snapshot_append_layout (snapshot, self->layout_.get (), &c2_rgba);
  gtk_snapshot_restore (snapshot);

  /* pop clip */
  gtk_snapshot_pop (snapshot);
}
