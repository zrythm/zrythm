// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 */

#include "dsp/audio_bus_track.h"
#include "dsp/channel.h"
#include "dsp/chord_track.h"
#include "dsp/instrument_track.h"
#include "dsp/region.h"
#include "dsp/track.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "utils/color.h"
#include "utils/gtk.h"
#include "utils/ui.h"
#include "zrythm_app.h"

static void
recreate_pango_layouts (MidiNote * self, int width)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  if (!PANGO_IS_LAYOUT (self->layout))
    {
      self->layout = z_cairo_create_default_pango_layout (
        GTK_WIDGET (arranger_object_get_arranger (obj)));
    }
  pango_layout_set_width (self->layout, pango_units_from_double (width - 2));
}

/**
 * @param cr Arranger's cairo context.
 * @param arr_rect Arranger's rectangle.
 */
void
midi_note_draw (MidiNote * self, GtkSnapshot * snapshot)
{
  ArrangerObject * obj = (ArrangerObject *) self;

  /* get rects */
  GdkRectangle full_rect = obj->full_rect;

  /* get color */
  GdkRGBA color;
  midi_note_get_adjusted_color (self, &color);

  Track * tr = arranger_object_get_track ((ArrangerObject *) self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (tr));
  bool drum_mode = tr->drum_mode;

  /* create clip */
  GskRoundedRect  rounded_rect;
  graphene_rect_t graphene_rect = GRAPHENE_RECT_INIT (
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
      gdk_cairo_set_source_rgba (diamond_cr, &color);
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
          GdkRGBA transparent_color = color;
          transparent_color.alpha = 0.5f;
          gtk_snapshot_append_color (
            snapshot, &transparent_color, &graphene_rect);
          /* draw velocity-filled part */
          graphene_rect_t graphene_vel_rect = GRAPHENE_RECT_INIT (
            (float) full_rect.x, (float) full_rect.y,
            (float) full_rect.width * ((float) self->vel->vel / 128.f),
            (float) full_rect.height);
          gtk_snapshot_append_color (snapshot, &color, &graphene_vel_rect);
        }
      else
        {
          gtk_snapshot_append_color (snapshot, &color, &graphene_rect);
        }
    }

  /* draw text */
  char str[30];
  midi_note_get_val_as_string (
    self, str,
    (PianoRollNoteNotation) g_settings_get_enum (
      S_UI, "piano-roll-note-notation"),
    1);
  int fontsize = piano_roll_keys_widget_get_font_size (MW_PIANO_ROLL_KEYS);
  if ((DEBUGGING || !drum_mode) && fontsize > 10)
    {
      char fontize_str[120];
      sprintf (
        fontize_str, "<span size=\"%d\">%s</span>",
        /* subtract half a point for the padding */
        fontsize * 1000 - 4000, str);

      double fontsize_ratio = (double) fontsize / 12.0;

      GdkRGBA c2;
      ui_get_contrast_color (&color, &c2);

      recreate_pango_layouts (self, MIN (full_rect.width, 400));
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt = GRAPHENE_POINT_INIT (
          REGION_NAME_BOX_PADDING + (float) full_rect.x,
          (float) (fontsize_ratio * REGION_NAME_BOX_PADDING
                   + (float) full_rect.y));
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      pango_layout_set_markup (self->layout, fontize_str, -1);
      gtk_snapshot_append_layout (snapshot, self->layout, &c2);
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
midi_note_get_adjusted_color (MidiNote * self, GdkRGBA * color)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  ArrangerWidget * arranger = arranger_object_get_arranger (obj);
  ZRegion *        region = arranger_object_get_region (obj);
  ZRegion *        ce_region = clip_editor_get_region (CLIP_EDITOR);
  Position         global_start_pos;
  midi_note_get_global_start_pos (self, &global_start_pos);
  ChordObject * co =
    chord_track_get_chord_at_pos (P_CHORD_TRACK, &global_start_pos);
  ScaleObject * so =
    chord_track_get_scale_at_pos (P_CHORD_TRACK, &global_start_pos);
  int  normalized_key = self->val % 12;
  bool in_scale =
    so
    && musical_scale_contains_note (
      so->scale, ENUM_INT_TO_VALUE (MusicalNote, normalized_key));
  bool in_chord =
    co
    && chord_descriptor_is_key_in_chord (
      chord_object_get_chord_descriptor (co),
      ENUM_INT_TO_VALUE (MusicalNote, normalized_key));
  bool is_bass =
    co
    && chord_descriptor_is_key_bass (
      chord_object_get_chord_descriptor (co),
      ENUM_INT_TO_VALUE (MusicalNote, normalized_key));

  /* get color */
  if (
    (PIANO_ROLL->highlighting == PianoRollHighlighting::PR_HIGHLIGHT_BOTH
     || PIANO_ROLL->highlighting == PianoRollHighlighting::PR_HIGHLIGHT_CHORD)
    && is_bass)
    {
      *color = UI_COLORS->highlight_bass_bg;
    }
  else if (
    PIANO_ROLL->highlighting == PianoRollHighlighting::PR_HIGHLIGHT_BOTH
    && in_scale && in_chord)
    {
      *color = UI_COLORS->highlight_both_bg;
    }
  else if (
    (PIANO_ROLL->highlighting == PianoRollHighlighting::PR_HIGHLIGHT_SCALE
     || PIANO_ROLL->highlighting == PianoRollHighlighting::PR_HIGHLIGHT_BOTH)
    && in_scale)
    {
      *color = UI_COLORS->highlight_scale_bg;
    }
  else if (
    (PIANO_ROLL->highlighting == PianoRollHighlighting::PR_HIGHLIGHT_CHORD
     || PIANO_ROLL->highlighting == PianoRollHighlighting::PR_HIGHLIGHT_BOTH)
    && in_chord)
    {
      *color = UI_COLORS->highlight_chord_bg;
    }
  else
    {
      Track * track = arranger_object_get_track (obj);
      *color = track->color;
    }

  /*
   * adjust color so that velocity highlight/
   * selection color is visible
   * - if color is black, make it lighter
   * - if color is white, make it darker
   * - if color is too dark, make it lighter
   * - if color is too bright, make it darker
   */
  if (color_is_very_very_dark (color))
    color_brighten (color, 0.7f);
  else if (color_is_very_very_bright (color))
    color_darken (color, 0.3f);
  else if (color_is_very_dark (color))
    color_brighten (color, 0.05f);
  else if (color_is_very_bright (color))
    color_darken (color, 0.05f);

  /* adjust color for velocity */
  GdkRGBA max_vel_color = *color;
  color_brighten (&max_vel_color, color_get_darkness (color) * 0.1f);
  GdkRGBA grey = *color;
  color_darken (&grey, color_get_brightness (color) * 0.6f);
  float vel_multiplier = self->vel->vel / 127.f;
  color_morph (&grey, &max_vel_color, vel_multiplier, color);

  /* also morph into grey */
  grey.red = 0.5f;
  grey.green = 0.5f;
  grey.blue = 0.5f;
  color_morph (&grey, color, MIN (vel_multiplier + 0.4f, 1.f), color);

  /* draw notes of main region */
  if (region == ce_region)
    {
      /* get color */
      ui_get_arranger_object_color (
        color, arranger->hovered_object == obj, midi_note_is_selected (self),
        false, arranger_object_get_muted (obj, false));
    }
  /* draw other notes */
  else
    {
      /* get color */
      ui_get_arranger_object_color (
        color, arranger->hovered_object == obj, midi_note_is_selected (self),
        /* FIXME */
        false, arranger_object_get_muted (obj, false));
      color_darken_default (color);
      color->alpha = 0.2f;
    }
}
