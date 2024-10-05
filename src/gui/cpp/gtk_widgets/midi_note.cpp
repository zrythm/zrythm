// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/settings/g_settings_manager.h"
#include "gui/cpp/gtk_widgets/arranger.h"
#include "gui/cpp/gtk_widgets/bot_bar.h"
#include "gui/cpp/gtk_widgets/bot_dock_edge.h"
#include "gui/cpp/gtk_widgets/center_dock.h"
#include "gui/cpp/gtk_widgets/clip_editor.h"
#include "gui/cpp/gtk_widgets/clip_editor_inner.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/midi_arranger.h"
#include "gui/cpp/gtk_widgets/midi_editor_space.h"
#include "gui/cpp/gtk_widgets/midi_note.h"
#include "gui/cpp/gtk_widgets/piano_roll_keys.h"
#include "gui/cpp/gtk_widgets/region.h"
#include "gui/cpp/gtk_widgets/ruler.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include "common/dsp/chord_track.h"
#include "common/dsp/region.h"
#include "common/dsp/track.h"
#include "common/utils/cairo.h"
#include "common/utils/color.h"
#include "common/utils/gtk.h"
#include "common/utils/ui.h"

static void
recreate_pango_layouts (MidiNote * self, int width)
{
  if (!PANGO_IS_LAYOUT (self->layout_.get ()))
    {
      self->layout_ = PangoLayoutUniquePtr (z_cairo_create_default_pango_layout (
        GTK_WIDGET (self->get_arranger ())));
    }
  pango_layout_set_width (
    self->layout_.get (), pango_units_from_double (width - 2));
}

void
midi_note_draw (MidiNote * self, GtkSnapshot * snapshot)
{
  /* get rects */
  GdkRectangle full_rect = self->full_rect_;

  /* get color */
  Color color;
  midi_note_get_adjusted_color (self, color);

  auto tr = dynamic_cast<PianoRollTrack *> (self->get_track ());
  z_return_if_fail (tr);
  bool drum_mode = tr->drum_mode_;

  /* create clip */
  GskRoundedRect  rounded_rect;
  graphene_rect_t graphene_rect = Z_GRAPHENE_RECT_INIT (
    (float) full_rect.x, (float) full_rect.y, (float) full_rect.width,
    (float) full_rect.height);
  if (drum_mode)
    {
      gtk_snapshot_push_clip (snapshot, &graphene_rect);
    }
  else
    {
      /* clip rounded rectangle for whole note */
      gsk_rounded_rect_init_from_rect (
        &rounded_rect, &graphene_rect, (float) full_rect.height / 6.0f);
      gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);
    }

  /* draw bg */
  cairo_t * diamond_cr = NULL;
  if (drum_mode)
    {
      diamond_cr = gtk_snapshot_append_cairo (snapshot, &graphene_rect);
      auto color_rgba = color.to_gdk_rgba ();
      gdk_cairo_set_source_rgba (diamond_cr, &color_rgba);
      /* translate to the full rect */
      cairo_translate (diamond_cr, (int) (full_rect.x), (int) (full_rect.y));
      z_cairo_diamond (diamond_cr, 0, 0, full_rect.width, full_rect.height);
      cairo_fill_preserve (diamond_cr);
    }
  else
    {
      /* TODO add option */
      const bool draw_with_velocities = false;
      if (draw_with_velocities)
        {
          /* draw full part */
          GdkRGBA transparent_color = color.to_gdk_rgba ();
          transparent_color.alpha = 0.5f;
          gtk_snapshot_append_color (
            snapshot, &transparent_color, &graphene_rect);
          /* draw velocity-filled part */
          graphene_rect_t graphene_vel_rect = Z_GRAPHENE_RECT_INIT (
            (float) full_rect.x, (float) full_rect.y,
            (float) full_rect.width * ((float) self->vel_->vel_ / 128.f),
            (float) full_rect.height);
          auto color_rgba = color.to_gdk_rgba ();
          gtk_snapshot_append_color (snapshot, &color_rgba, &graphene_vel_rect);
        }
      else
        {
          auto color_rgba = color.to_gdk_rgba ();
          gtk_snapshot_append_color (snapshot, &color_rgba, &graphene_rect);
        }
    }

  /* draw text */
  auto str = self->get_val_as_string (
    (PianoRoll::NoteNotation) g_settings_get_enum (
      S_UI, "piano-roll-note-notation"),
    true);
  int fontsize = piano_roll_keys_widget_get_font_size (MW_PIANO_ROLL_KEYS);
  if ((DEBUGGING || !drum_mode) && fontsize > 10)
    {
      auto fontize_str = fmt::format (
        "<span size=\"{}\">{}</span>",
        /* subtract half a point for the padding */
        fontsize * 1000 - 4000, str);

      double fontsize_ratio = (double) fontsize / 12.0;

      GdkRGBA c2 = color.get_contrast_color ().to_gdk_rgba ();

      recreate_pango_layouts (self, MIN (full_rect.width, 400));
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
          REGION_NAME_BOX_PADDING + (float) full_rect.x,
          (float) (fontsize_ratio * REGION_NAME_BOX_PADDING
                   + (float) full_rect.y));
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      pango_layout_set_markup (self->layout_.get (), fontize_str.c_str (), -1);
      gtk_snapshot_append_layout (snapshot, self->layout_.get (), &c2);
      gtk_snapshot_restore (snapshot);
    }

  /* add border */
  const float border_width = 1.f;
  GdkRGBA     border_color = { 0.5f, 0.5f, 0.5f, 0.4f };
  if (drum_mode)
    {
      gdk_cairo_set_source_rgba (diamond_cr, &border_color);
      cairo_stroke (diamond_cr);
    }
  else
    {
      float border_widths[] = {
        border_width, border_width, border_width, border_width
      };
      GdkRGBA border_colors[] = {
        border_color, border_color, border_color, border_color
      };
      gtk_snapshot_append_border (
        snapshot, &rounded_rect, border_widths, border_colors);
    }

  /* pop clip */
  gtk_snapshot_pop (snapshot);

  /* free temp cairo context */
  if (diamond_cr)
    cairo_destroy (diamond_cr);
}

void
midi_note_get_adjusted_color (MidiNote * self, Color &color)
{
  // ArrangerWidget * arranger = self->get_arranger ();
  auto     region = self->get_region ();
  auto     ce_region = CLIP_EDITOR->get_region ();
  Position global_start_pos;
  self->get_global_start_pos (global_start_pos);
  ChordObject * co = P_CHORD_TRACK->get_chord_at_pos (global_start_pos);
  ScaleObject * so = P_CHORD_TRACK->get_scale_at_pos (global_start_pos);
  int           normalized_key = self->val_ % 12;
  bool          in_scale =
    so
    && so->scale_.contains_note (ENUM_INT_TO_VALUE (MusicalNote, normalized_key));
  bool in_chord =
    co
    && co->get_chord_descriptor ()->is_key_in_chord (
      ENUM_INT_TO_VALUE (MusicalNote, normalized_key));
  bool is_bass =
    co
    && co->get_chord_descriptor ()->is_key_bass (
      ENUM_INT_TO_VALUE (MusicalNote, normalized_key));

  /* get color */
  if (
    (PIANO_ROLL->highlighting_ == PianoRoll::Highlighting::Both
     || PIANO_ROLL->highlighting_ == PianoRoll::Highlighting::Chord)
    && is_bass)
    {
      color = UI_COLORS->highlight_bass_bg;
    }
  else if (
    PIANO_ROLL->highlighting_ == PianoRoll::Highlighting::Both && in_scale
    && in_chord)
    {
      color = UI_COLORS->highlight_both_bg;
    }
  else if (
    (PIANO_ROLL->highlighting_ == PianoRoll::Highlighting::Scale
     || PIANO_ROLL->highlighting_ == PianoRoll::Highlighting::Both)
    && in_scale)
    {
      color = UI_COLORS->highlight_scale_bg;
    }
  else if (
    (PIANO_ROLL->highlighting_ == PianoRoll::Highlighting::Chord
     || PIANO_ROLL->highlighting_ == PianoRoll::Highlighting::Both)
    && in_chord)
    {
      color = UI_COLORS->highlight_chord_bg;
    }
  else
    {
      Track * track = self->get_track ();
      color = track->color_;
    }

  /*
   * adjust color so that velocity highlight/
   * selection color is visible
   * - if color is black, make it lighter
   * - if color is white, make it darker
   * - if color is too dark, make it lighter
   * - if color is too bright, make it darker
   */
  if (color.is_very_dark ())
    color.brighten (0.7f);
  else if (color.is_very_very_bright ())
    color.darken (0.3f);
  else if (color.is_very_dark ())
    color.brighten (0.05f);
  else if (color.is_very_bright ())
    color.darken (0.05f);

  /* adjust color for velocity */
  auto max_vel_color = color;
  max_vel_color.brighten (color.get_darkness () * 0.1f);
  auto grey = color;
  grey.darken (color.get_brightness () * 0.6f);
  float vel_multiplier = self->vel_->vel_ / 127.f;
  color = grey.morph (max_vel_color, vel_multiplier);

  /* also morph into grey */
  grey.red_ = 0.5f;
  grey.green_ = 0.5f;
  grey.blue_ = 0.5f;
  color = grey.morph (color, std::min (vel_multiplier + 0.4f, 1.f));

  /* draw notes of main region */
  if (region == ce_region)
    {
      /* get color */
      color = Color::get_arranger_object_color (
        color, self->is_hovered (), self->is_selected (), false,
        self->get_muted (false));
    }
  /* draw other notes */
  else
    {
      /* get color */
      color = Color::get_arranger_object_color (
        color, self->is_hovered (), self->is_selected (),
        /* FIXME */
        false, self->get_muted (false));
      color.darken_default ();
      color.alpha_ = 0.2f;
    }
}
