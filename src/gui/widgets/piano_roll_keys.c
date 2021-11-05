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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "audio/midi_event.h"
#include "audio/scale_object.h"
#include "audio/tracklist.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/piano_roll_keys.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/color.h"
#include "utils/gtk.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  PianoRollKeysWidget, piano_roll_keys_widget,
  GTK_TYPE_WIDGET)

#define DEFAULT_PX_PER_KEY 7
/* can also try Sans SemiBold */
#define PIANO_ROLL_KEYS_FONT "Sans 8"

static void
piano_roll_keys_snapshot (
  GtkWidget *   widget,
  GtkSnapshot * snapshot)
{
  PianoRollKeysWidget * self =
    Z_PIANO_ROLL_KEYS_WIDGET (widget);
  GtkScrolledWindow * scroll =
    MW_MIDI_EDITOR_SPACE->piano_roll_keys_scroll;
  graphene_rect_t visible_rect;
  z_gtk_scrolled_window_get_visible_rect (
    scroll, &visible_rect);
  GdkRectangle visible_rect_gdk;
  z_gtk_graphene_rect_t_to_gdk_rectangle (
    &visible_rect_gdk, &visible_rect);

  Track * tr =
    clip_editor_get_track (CLIP_EDITOR);
  if (!IS_TRACK_AND_NONNULL (tr))
    {
      return;
    }

  cairo_t * cr =
    gtk_snapshot_append_cairo (
      snapshot, &visible_rect);

  GtkStyleContext *context =
    gtk_widget_get_style_context (widget);
  int width =
    gtk_widget_get_allocated_width (widget);
  gtk_render_background (
    context, cr, 0, 0, width,
    visible_rect_gdk.height);

  ChordObject * co =
    chord_track_get_chord_at_playhead (
      P_CHORD_TRACK);
  ScaleObject * so =
    chord_track_get_scale_at_playhead (
      P_CHORD_TRACK);

  bool drum_mode = tr->drum_mode;

  double label_width =
    (double) width * 0.55;
  if (drum_mode)
    {
      label_width = width - 8;
    }
  double key_width =
    (double) width - label_width;
  double px_per_key = self->px_per_key + 1.0;

  char str[400];

  for (int i = 0; i < 128; i++)
    {
      /* skip keys outside visible rectangle */
      if ((double) visible_rect_gdk.y >
            (double) ((127 - i) + 1) * px_per_key ||
          (double) (visible_rect_gdk.y + visible_rect_gdk.height) <
            (double) (127 - i) * px_per_key)
        continue;

      /* check if in scale and in chord */
      int normalized_key = i % 12;
      bool in_scale =
        so &&
        musical_scale_is_key_in_scale (
          so->scale, normalized_key);
      bool in_chord =
        co &&
        chord_descriptor_is_key_in_chord (
          chord_object_get_chord_descriptor (co),
          normalized_key);
      bool is_bass =
        co &&
        chord_descriptor_is_key_bass (
          chord_object_get_chord_descriptor (co),
          normalized_key);

      /* ---- draw label ---- */

      const MidiNoteDescriptor * descr =
        piano_roll_find_midi_note_descriptor_by_val (
          PIANO_ROLL, drum_mode, i);

      char fontize_str[120];
      int fontsize =
        piano_roll_keys_widget_get_font_size (
          self);
      sprintf (
        fontize_str, "<span size=\"%d\">",
        fontsize * 1000 - 4000);
      strcat (fontize_str, "%s</span>");
      char note_name[120];
      sprintf (
        note_name, fontize_str,
        drum_mode ?
          descr->custom_name :
          descr->note_name_pango);

      if (drum_mode)
        {
          strcpy (str, note_name);
        }
      else
        {
          /* ---- draw background ---- */
          int has_color = 0;
          if ((PIANO_ROLL->highlighting ==
                 PR_HIGHLIGHT_BOTH ||
               PIANO_ROLL->highlighting ==
                 PR_HIGHLIGHT_CHORD) &&
              is_bass)
            {
              has_color = 1;

              gdk_cairo_set_source_rgba (
                cr, &UI_COLORS->highlight_bass_bg);
              cairo_rectangle (
                cr, 0,
                (127 - i) * px_per_key,
                label_width, px_per_key);
              cairo_fill (cr);

              char hex[18];
              ui_gdk_rgba_to_hex (
                &UI_COLORS->highlight_bass_fg, hex);
              sprintf (
                str,
                "%s  <span size=\"small\" foreground=\"%s\">%s</span>",
                note_name, hex, _("bass"));
            }
          else if (PIANO_ROLL->highlighting ==
                PR_HIGHLIGHT_BOTH && in_chord &&
                in_scale)
            {
              has_color = 1;

              gdk_cairo_set_source_rgba (
                cr, &UI_COLORS->highlight_both_bg);
              cairo_rectangle (
                cr, 0,
                (127 - i) * px_per_key,
                label_width, px_per_key);
              cairo_fill (cr);

              char hex[18];
              ui_gdk_rgba_to_hex (
                &UI_COLORS->highlight_both_fg, hex);
              sprintf (
                str,
                "%s  <span size=\"small\" foreground=\"%s\">%s</span>",
                note_name, hex, _("both"));
            }
          else if ((PIANO_ROLL->highlighting ==
                PR_HIGHLIGHT_SCALE ||
              PIANO_ROLL->highlighting ==
                PR_HIGHLIGHT_BOTH) && in_scale)
            {
              has_color = 1;

              gdk_cairo_set_source_rgba (
                cr, &UI_COLORS->highlight_scale_bg);

              char hex[18];
              ui_gdk_rgba_to_hex (
                &UI_COLORS->highlight_scale_fg, hex);
              sprintf (
                str,
                "%s  <span size=\"small\" foreground=\"%s\">%s</span>",
                note_name, hex, _("scale"));
            }
          else if ((PIANO_ROLL->highlighting ==
                PR_HIGHLIGHT_CHORD ||
              PIANO_ROLL->highlighting ==
                PR_HIGHLIGHT_BOTH) && in_chord)
            {
              has_color = 1;

              gdk_cairo_set_source_rgba (
                cr, &UI_COLORS->highlight_chord_bg);

              char hex[18];
              ui_gdk_rgba_to_hex (
                &UI_COLORS->highlight_chord_fg, hex);
              sprintf (
                str,
                "%s  <span size=\"small\" foreground=\"%s\">%s</span>",
                note_name, hex, _("chord"));
            }
          else
            {
              strcpy (str, note_name);
            }

          /* draw the background color if any */
          if (has_color)
            {
              cairo_rectangle (
                cr, 0,
                (127 - i) * px_per_key,
                label_width, px_per_key);
              cairo_fill (cr);
            }
        }

      /* ---- draw label ---- */

      /* only show text if large enough */
      if (px_per_key > 16.0)
        {
          g_return_if_fail (self->layout);
          cairo_set_source_rgba (
            cr, 1, 1, 1, 1);
          pango_layout_set_markup (
            self->layout, str, -1);
          int ww, hh;
          pango_layout_get_pixel_size (
            self->layout, &ww, &hh);
          double text_y_start =
            /* y start of the note */
            (127 - i) * px_per_key +
            /* half of the space remaining excluding
             * hh */
            (px_per_key - (double) hh) / 2.0;
          cairo_move_to (
            cr, 4, text_y_start);
          pango_cairo_show_layout (
            cr, self->layout);
        }

      /* ---- draw key ---- */

      /* draw note */
      int black_note =
        piano_roll_is_key_black (i);

      if (black_note)
        cairo_set_source_rgb (cr, 0, 0, 0);
      else
        cairo_set_source_rgb (cr, 1, 1, 1);
      cairo_rectangle (
        cr, label_width, (127 - i) * px_per_key,
        key_width, px_per_key);
      cairo_fill (cr);

      /* add shade if currently pressed note */
      if (piano_roll_contains_current_note (
            PIANO_ROLL, i))
        {
          /* orange */
          cairo_set_source_rgba (
            cr, 1, 0.462745, 0.101961, 1);
#if 0
          /* sky blue */
          cairo_set_source_rgba (
            cr, 0.101961, 0.63922, 1, 1);
#endif
          cairo_rectangle (
            cr,
            label_width + 4, (127 - i) * px_per_key,
            key_width - 4, px_per_key);
          cairo_fill (cr);
        }

      /* add border below */
      double y = ((127 - i) + 1) * px_per_key;
      cairo_set_source_rgba (
        cr, 0.7, 0.7, 0.7, 1.0);
      cairo_set_line_width (cr, 0.3);
      cairo_move_to (cr, 0, y);
      cairo_line_to (cr, width, y);
      cairo_stroke (cr);
    }

  cairo_destroy (cr);
}

int
piano_roll_keys_widget_get_key_from_y (
  PianoRollKeysWidget * self,
  double                y)
{
  return
    127 -
    (int)
      (y / (self->px_per_key + 1.0));
}

/**
 * Returns the appropriate font size based on the
 * current pixels (height) per key.
 */
int
piano_roll_keys_widget_get_font_size (
  PianoRollKeysWidget * self)
{
  /* converted from pixels to points */
  /* see https://websemantics.uk/articles/font-size-conversion/ */
  if (self->px_per_key >= 16.0)
    return 12;
  else if (self->px_per_key >= 13.0)
    return 10;
  else if (self->px_per_key >= 10.0)
    return 7;
  else
    return 6;
  g_return_val_if_reached (-1);
}

static void
send_note_event (
  PianoRollKeysWidget * self,
  int                   note,
  int                   on)
{
  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  if (on)
    {
      /* add note on event */
      midi_events_add_note_on (
        MANUAL_PRESS_EVENTS,
        midi_region_get_midi_ch (region),
        (midi_byte_t) note, 90, 1, 1);

      piano_roll_add_current_note (
        PIANO_ROLL, note);
    }
  else
    {
      /* add note off event */
      midi_events_add_note_off (
        MANUAL_PRESS_EVENTS,
        midi_region_get_midi_ch (region),
        (midi_byte_t) note, 1, 1);

      piano_roll_remove_current_note (
        PIANO_ROLL, note);
    }

  piano_roll_keys_widget_redraw_note (
    self, note);
}

static void
on_motion (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  PianoRollKeysWidget *      self)
{
  int key =
    piano_roll_keys_widget_get_key_from_y (self, y);

  if (self->note_pressed &&
      !self->note_released)
    {
      if (self->last_key != key)
        {
          send_note_event (self, self->last_key, 0);
          send_note_event (self, key, 1);
        }
      self->last_key = key;
    }

  self->last_hovered_key = key;
  /*const MidiNoteDescriptor * descr =*/
    /*piano_roll_find_midi_note_descriptor_by_val (*/
      /*PIANO_ROLL, key);*/
  /*g_message ("hovered %s",*/
    /*descr->note_name);*/
}

static void
on_pressed (
  GtkGestureClick *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  PianoRollKeysWidget * self)
{
  self->note_pressed = 1;
  self->note_released = 0;

  int key =
    piano_roll_keys_widget_get_key_from_y (
      self, y);
  self->last_key = key;
  self->start_key = key;
  send_note_event (self, key, 1);
}

static void
on_released (
  GtkGestureClick *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  PianoRollKeysWidget *  self)
{
  self->note_pressed = 0;
  self->note_released = 1;
  if (self->last_key)
    send_note_event (self, self->last_key, 0);
  self->last_key = -1;
}

void
piano_roll_keys_widget_refresh (
  PianoRollKeysWidget * self)
{
  self->px_per_key =
    (double) DEFAULT_PX_PER_KEY *
    (double) PIANO_ROLL->notes_zoom;
  self->total_key_px =
    (self->px_per_key + 1.0) * 128.0;

  gtk_widget_set_size_request (
    GTK_WIDGET (self), -1, (int) self->total_key_px);

  piano_roll_keys_widget_redraw_full (self);
}

void
piano_roll_keys_widget_redraw_note (
  PianoRollKeysWidget * self,
  int                   note)
{
  /* TODO */
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
piano_roll_keys_widget_redraw_full (
  PianoRollKeysWidget * self)
{
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
piano_roll_keys_widget_setup (
  PianoRollKeysWidget * self)
{
}

static void
piano_roll_keys_widget_init (
  PianoRollKeysWidget * self)
{
  self->last_mid_note = 63;

  PangoFontDescription * desc;
  self->layout =
    gtk_widget_create_pango_layout (
      GTK_WIDGET (self), NULL);
  desc =
    pango_font_description_from_string (
      PIANO_ROLL_KEYS_FONT);
  pango_layout_set_font_description (
    self->layout, desc);
  pango_font_description_free (desc);

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (
      gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion_controller), "motion",
    G_CALLBACK (on_motion), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (motion_controller));

  self->multipress =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  g_signal_connect (
    G_OBJECT (self->multipress), "pressed",
    G_CALLBACK (on_pressed),  self);
  g_signal_connect (
    G_OBJECT (self->multipress), "released",
    G_CALLBACK (on_released),  self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->multipress));
}

static void
piano_roll_keys_widget_class_init (
  PianoRollKeysWidgetClass * _klass)
{
  GtkWidgetClass * wklass =
    GTK_WIDGET_CLASS (_klass);
  wklass->snapshot = piano_roll_keys_snapshot;
}
