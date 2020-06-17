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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "audio/scale_object.h"
#include "audio/tracklist.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/piano_roll_keys.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  PianoRollKeysWidget, piano_roll_keys_widget,
  GTK_TYPE_DRAWING_AREA)

#define DEFAULT_PX_PER_KEY 7
#define DRUM_MODE (PIANO_ROLL->drum_mode)
/* can also try Sans SemiBold */
#define PIANO_ROLL_KEYS_FONT "Sans 8"

static gboolean
piano_roll_keys_draw (
  GtkWidget * widget,
  cairo_t *   cr,
  PianoRollKeysWidget * self)
{
  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (cr, &rect);

  GtkStyleContext *context =
    gtk_widget_get_style_context (widget);
  int width = gtk_widget_get_allocated_width (widget);
  gtk_render_background (
    context, cr, 0, 0, width, rect.height);

  ChordObject * co =
    chord_track_get_chord_at_playhead (
      P_CHORD_TRACK);
  ScaleObject * so =
    chord_track_get_scale_at_playhead (
      P_CHORD_TRACK);

  double label_width =
    (double) width * 0.55;
  double key_width =
    (double) width - label_width;
  double px_per_key = self->px_per_key + 1.0;

  char str[400];

  for (int i = 0; i < 128; i++)
    {
      /* skip keys outside visible rectangle */
      if ((double) rect.y >
            (double) ((127 - i) + 1) * px_per_key ||
          (double) (rect.y + rect.height) <
            (double) (127 - i) * px_per_key)
        continue;

      if (DRUM_MODE)
        {
        }
      else
        {
          /* ---- draw label ---- */

          const MidiNoteDescriptor * descr =
            piano_roll_find_midi_note_descriptor_by_val (
              PIANO_ROLL, i);

          char fontize_str[120];
          int fontsize =
            piano_roll_keys_widget_get_font_size (
              self);
          sprintf (
            fontize_str, "<span size=\"%d\">",
            fontsize * 1000 - 4000);
          strcat (fontize_str, "%s</span>");

          /* highlight if in chord/scale */
          int in_scale =
            so && musical_scale_is_key_in_scale (
              so->scale, i % 12);
          int in_chord =
            co && chord_descriptor_is_key_in_chord (
              chord_object_get_chord_descriptor (co),
              i % 12);

          char note_name[120];
          sprintf (
            note_name, fontize_str,
            descr->note_name_pango);

          int has_color = 0;
          if (PIANO_ROLL->highlighting ==
                PR_HIGHLIGHT_BOTH && in_chord &&
                in_scale)
            {
              has_color = 1;

              gdk_cairo_set_source_rgba (
                cr, &UI_COLORS->highlight_both);
              cairo_rectangle (
                cr, 0,
                (127 - i) * px_per_key,
                label_width, px_per_key);
              cairo_fill (cr);

              sprintf (
                str,
                "%s  <span size=\"small\" foreground=\"#F79616\">%s</span>",
                descr->note_name_pango,
                _("both"));
            }
          else if ((PIANO_ROLL->highlighting ==
                PR_HIGHLIGHT_SCALE ||
              PIANO_ROLL->highlighting ==
                PR_HIGHLIGHT_BOTH) && in_scale)
            {
              has_color = 1;

              gdk_cairo_set_source_rgba (
                cr, &UI_COLORS->highlight_in_scale);

              sprintf (
                str,
                "%s  <span size=\"small\" foreground=\"#F79616\">%s</span>",
                note_name,
                _("scale"));
            }
          else if ((PIANO_ROLL->highlighting ==
                PR_HIGHLIGHT_CHORD ||
              PIANO_ROLL->highlighting ==
                PR_HIGHLIGHT_BOTH) && in_chord)
            {
              has_color = 1;

              gdk_cairo_set_source_rgba (
                cr, &UI_COLORS->highlight_in_chord);

              sprintf (
                str,
                "%s  <span size=\"small\" foreground=\"#F79616\">%s</span>",
                note_name,
                _("chord"));
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

          /* only show text if large enough */
          if (px_per_key > 16.0)
            {
              g_return_val_if_fail (
                self->layout, FALSE);
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
              if (black_note)
                cairo_set_source_rgba (
                  cr, 1, 1, 1, 0.1);
              else
                cairo_set_source_rgba (
                  cr, 0, 0, 0, 0.3);
              cairo_rectangle (
                cr, label_width, (127 - i) * px_per_key,
                key_width, px_per_key);
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
    }

  return FALSE;
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

static gboolean
on_motion (
  GtkWidget *widget,
  GdkEventMotion  *event,
  PianoRollKeysWidget * self)
{
  int key =
    piano_roll_keys_widget_get_key_from_y (
      self, event->y);

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

  return FALSE;
}

static void
on_pressed (
  GtkGestureMultiPress *gesture,
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
  GtkGestureMultiPress *gesture,
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
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (piano_roll_keys_draw), self);
}

static void
piano_roll_keys_widget_init (
  PianoRollKeysWidget * self)
{
  self->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));

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

  /* make it able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  /* setup signals */
  g_signal_connect (
    G_OBJECT (self), "motion-notify-event",
    G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT(self->multipress), "pressed",
    G_CALLBACK (on_pressed),  self);
  g_signal_connect (
    G_OBJECT(self->multipress), "released",
    G_CALLBACK (on_released),  self);
}

static void
piano_roll_keys_widget_class_init (
  PianoRollKeysWidgetClass * _klass)
{
}
