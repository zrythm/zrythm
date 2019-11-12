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

/**
 * \file
 *
 */

#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/instrument_track.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "utils/ui.h"

G_DEFINE_TYPE (
  MidiNoteWidget, midi_note_widget,
  ARRANGER_OBJECT_WIDGET_TYPE)

/**
 * Space on the edges to show resize cursors
 */
#define RESIZE_CURSOR_SPACE 9

static gboolean
midi_note_draw_cb (
  GtkWidget *      widget,
  cairo_t *        cr,
  MidiNoteWidget * self)
{
  if (!GTK_IS_WIDGET (self))
    return FALSE;

  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  /*GdkRectangle rect;*/
  /*gdk_cairo_get_clip_rectangle (*/
    /*cr, &rect);*/

  if (ao_prv->redraw)
    {
      GtkStyleContext *context =
        gtk_widget_get_style_context (widget);

      int width =
        gtk_widget_get_allocated_width (widget);
      int height =
        gtk_widget_get_allocated_height (widget);

      z_cairo_reset_caches (
        &ao_prv->cached_cr,
        &ao_prv->cached_surface, width,
        height, cr);

      gtk_render_background (
        context, ao_prv->cached_cr, 0, 0,
        width, height);

      MidiNote * mn = self->midi_note;
      Region * region = mn->region;
      Position global_start_pos;
      midi_note_get_global_start_pos (
        mn, &global_start_pos);
      ChordObject * co =
        chord_track_get_chord_at_pos (
          P_CHORD_TRACK, &global_start_pos);
      ScaleObject * so =
        chord_track_get_scale_at_pos (
          P_CHORD_TRACK, &global_start_pos);
      int in_scale =
        so && musical_scale_is_key_in_scale (
          so->scale, mn->val % 12);
      int in_chord =
        co && chord_descriptor_is_key_in_chord (
          chord_object_get_chord_descriptor (co),
          mn->val % 12);

      GdkRGBA color;
      if (PIANO_ROLL->highlighting ==
            PR_HIGHLIGHT_BOTH &&
          in_scale && in_chord)
        {
          color = UI_COLORS->highlight_both;
        }
      else if ((PIANO_ROLL->highlighting ==
            PR_HIGHLIGHT_SCALE ||
          PIANO_ROLL->highlighting ==
            PR_HIGHLIGHT_BOTH) && in_scale)
        {
          color = UI_COLORS->highlight_in_scale;
        }
      else if ((PIANO_ROLL->highlighting ==
            PR_HIGHLIGHT_CHORD ||
          PIANO_ROLL->highlighting ==
            PR_HIGHLIGHT_BOTH) && in_chord)
        {
          color = UI_COLORS->highlight_in_chord;
        }
      else
        {
          color =
            region->lane->track->color;
        }

      /* draw notes of main region */
      if (region == CLIP_EDITOR->region)
        {
          /* get color */
          ui_get_arranger_object_color (
            &color,
            gtk_widget_get_state_flags (
              GTK_WIDGET (self)) &
              GTK_STATE_FLAG_PRELIGHT,
            midi_note_is_selected (self->midi_note),
            midi_note_is_transient (self->midi_note));
          gdk_cairo_set_source_rgba (
            ao_prv->cached_cr, &color);

          if (PIANO_ROLL->drum_mode)
            {
              z_cairo_diamond (
                ao_prv->cached_cr, 0, 0,
                width, height);
            }
          else
            {
              z_cairo_rounded_rectangle (
                ao_prv->cached_cr,
                0, 0, width, height,
                1.0, 4.0);
            }
          cairo_fill (ao_prv->cached_cr);
        }
      /* draw other notes */
      else
        {
          /* get color */
          ui_get_arranger_object_color (
            &color,
            gtk_widget_get_state_flags (
              GTK_WIDGET (self)) &
              GTK_STATE_FLAG_PRELIGHT,
            midi_note_is_selected (self->midi_note),
            midi_note_is_transient (self->midi_note));
          color.alpha = 0.5;
          gdk_cairo_set_source_rgba (
            ao_prv->cached_cr, &color);

          if (PIANO_ROLL->drum_mode)
            {
              z_cairo_diamond (
                ao_prv->cached_cr,
                0, 0, width, height);
            }
          else
            {
              z_cairo_rounded_rectangle (
                ao_prv->cached_cr,
                0, 0, width, height,
                1.0, 4.0);
            }
          cairo_fill (ao_prv->cached_cr);
        }

      char str[30];
      midi_note_get_val_as_string (mn, str, 1);
      if (DEBUGGING &&
          midi_note_is_transient (self->midi_note))
        {
          strcat (str, " [t]");
        }

      GdkRGBA c2;
      ui_get_contrast_color (
        &color, &c2);
      gdk_cairo_set_source_rgba (
        ao_prv->cached_cr, &c2);
      if (DEBUGGING || !PIANO_ROLL->drum_mode)
        {
          PangoLayout * layout =
            z_cairo_create_default_pango_layout (
              widget);
          z_cairo_draw_text (
            ao_prv->cached_cr, widget, layout, str);
          g_object_unref (layout);
        }

      ao_prv->redraw = 0;
    }

  cairo_set_source_surface (
    cr, ao_prv->cached_surface, 0, 0);
  cairo_paint (cr);

 return FALSE;
}

MidiNoteWidget *
midi_note_widget_new (MidiNote * midi_note)
{
  MidiNoteWidget * self =
    g_object_new (MIDI_NOTE_WIDGET_TYPE,
                  "visible", 1,
                  NULL);

  arranger_object_widget_setup (
    Z_ARRANGER_OBJECT_WIDGET (self),
    (ArrangerObject *) midi_note);

  self->midi_note = midi_note;

  /* set tooltip text */
  char * tooltip =
    g_strdup_printf (
      "[%s%d] %d",
      chord_descriptor_note_to_string (
        self->midi_note->val % 12),
      self->midi_note->val / 12 - 1,
      self->midi_note->vel->vel);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), tooltip);
  g_free (tooltip);

  return self;
}

static void
midi_note_widget_class_init (
  MidiNoteWidgetClass * _klass)
{
  /*GtkWidgetClass * klass =*/
    /*GTK_WIDGET_CLASS (_klass);*/
}

static void
midi_note_widget_init (MidiNoteWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (ao_prv->drawing_area), "draw",
    G_CALLBACK (midi_note_draw_cb), self);
}
