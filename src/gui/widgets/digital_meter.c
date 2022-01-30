/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>
#include <math.h>

#include "audio/control_port.h"
#include "audio/position.h"
#include "audio/quantize_options.h"
#include "audio/router.h"
#include "audio/snap_grid.h"
#include "audio/tempo_track.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  DigitalMeterWidget, digital_meter_widget,
  GTK_TYPE_WIDGET)

#define FONT_SIZE 16
/*#define SEG7_FONT "Segment7 Bold 16"*/
#define SEG7_FONT "DSEG14 Classic Mini Italic 12"
#define NORMAL_FONT "sans-serif Bold 11"
#define CAPTION_FONT "sans-serif 7"
#define SPACE_BETWEEN 6
#define HALF_SPACE_BETWEEN 2
#define PADDING_W 4
#define PADDING_TOP 0

#define DISPLAY_TIME \
  (self->is_transport && \
   g_settings_get_enum ( \
     S_UI, "transport-display") == \
     TRANSPORT_DISPLAY_TIME)

#define SET_POS \
  ((*self->setter) (self->obj, &pos))
#define GET_POS \
  ((*self->getter) (self->obj, &pos))

static void
recreate_pango_layouts (
  DigitalMeterWidget * self)
{
  if (PANGO_IS_LAYOUT (self->caption_layout))
    g_object_unref (self->caption_layout);
  if (PANGO_IS_LAYOUT (self->seg7_layout))
    g_object_unref (self->seg7_layout);
  if (PANGO_IS_LAYOUT (self->normal_layout))
    g_object_unref (self->normal_layout);

  self->caption_layout =
    z_cairo_create_pango_layout_from_string (
      GTK_WIDGET (self), CAPTION_FONT,
      PANGO_ELLIPSIZE_NONE, -1);
  self->seg7_layout =
    z_cairo_create_pango_layout_from_string (
      GTK_WIDGET (self), SEG7_FONT,
      PANGO_ELLIPSIZE_NONE, -1);
  self->normal_layout =
    z_cairo_create_pango_layout_from_string (
      GTK_WIDGET (self), NORMAL_FONT,
      PANGO_ELLIPSIZE_NONE, -1);
}

static void
init_dm (
  DigitalMeterWidget * self)
{
  g_return_if_fail (Z_DIGITAL_METER_WIDGET (self));

  recreate_pango_layouts (self);

  int caption_textw, caption_texth;
  z_cairo_get_text_extents_for_widget (
    self, self->caption_layout, self->caption,
    &caption_textw, &caption_texth);
  int textw, texth;
  switch (self->type)
    {
    case DIGITAL_METER_TYPE_BPM:
      {
        gtk_widget_set_tooltip_text (
          (GtkWidget *) self, _("Tempo/BPM"));
        z_cairo_get_text_extents_for_widget (
          self, self->seg7_layout, "888888",
          &textw, &texth);
        /* caption + padding between caption and
         * BPM + padding top/bottom */
        gtk_widget_set_size_request (
          GTK_WIDGET (self), textw + PADDING_W * 2,
          caption_texth + HALF_SPACE_BETWEEN +
          texth + PADDING_TOP * 2);
      }
      break;
    case DIGITAL_METER_TYPE_POSITION:
      gtk_widget_set_tooltip_text (
        (GtkWidget *) self, _("Position"));
      z_cairo_get_text_extents_for_widget (
        self, self->seg7_layout, "-888888888",
        &textw, &texth);
      /* caption + padding between caption and
       * BPM + padding top/bottom */
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        textw + PADDING_W * 2 + HALF_SPACE_BETWEEN * 3,
        caption_texth + HALF_SPACE_BETWEEN +
          texth + PADDING_TOP * 2);

      break;
    case DIGITAL_METER_TYPE_NOTE_LENGTH:
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        -1,
        30);
      break;

    case DIGITAL_METER_TYPE_NOTE_TYPE:
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        -1,
        30);
      break;
    case DIGITAL_METER_TYPE_TIMESIG:
      {
        gtk_widget_set_tooltip_text (
          GTK_WIDGET (self),
          _("Time Signature - Beats per bar / "
            "Beat unit"));
        z_cairo_get_text_extents_for_widget (
          self, self->seg7_layout, "16/16",
          &textw, &texth);
        /* caption + padding between caption and
         * BPM + padding top/bottom */
        gtk_widget_set_size_request (
          GTK_WIDGET (self), textw + PADDING_W * 2,
          caption_texth + HALF_SPACE_BETWEEN +
          texth + PADDING_TOP * 2);
      }
      break;
    }

  self->initialized = true;
}


static void
digital_meter_snapshot (
  GtkWidget *   widget,
  GtkSnapshot * snapshot)
{
  DigitalMeterWidget * self =
    Z_DIGITAL_METER_WIDGET (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  gtk_snapshot_render_background (
    snapshot, context, 0, 0, width, height);

  if (!PROJECT->loaded)
    return;

  if (!self->initialized)
    init_dm (self);

  /* draw caption and get its extents */
  int caption_textw, caption_texth;
  GdkRGBA color;
  if (gtk_widget_is_sensitive (GTK_WIDGET (self)))
    {
      color = Z_GDK_RGBA_INIT (1, 1, 1, 1);
    }
  else
    {
      color = Z_GDK_RGBA_INIT (0.6f, 0.6f, 0.6f, 1);
    }
  z_cairo_get_text_extents_for_widget (
    self, self->caption_layout, self->caption,
    &caption_textw, &caption_texth);
  pango_layout_set_markup (
    self->caption_layout, self->caption, -1);
  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (
    snapshot,
    &GRAPHENE_POINT_INIT (
      width / 2.f - caption_textw / 2.f,
      PADDING_TOP));
  gtk_snapshot_append_layout (
    snapshot, self->caption_layout, &color);
  gtk_snapshot_restore (snapshot);

  /* draw line */
  if (self->draw_line)
    {
      gtk_snapshot_append_color (
        snapshot, &color,
        &GRAPHENE_RECT_INIT (
          0, caption_texth + PADDING_TOP,
          width, 1));
    }

  /*GdkRGBA color;*/
  /*gdk_rgba_parse (&color, "#D68A0C");*/
  /*gdk_cairo_set_source_rgba (cr, &color);*/
  if (gtk_widget_is_sensitive (GTK_WIDGET (self)))
    {
      color = Z_GDK_RGBA_INIT (0.f, 1.f, 0.1f, 1.f);
    }
  else
    {
      color =
        Z_GDK_RGBA_INIT (0.f, 0.6f, 0.06f, 1.f);
    }
  char text[20];
  char * heap_text = NULL;
  int num_part, dec_part, bars, beats, sixteenths, ticks;
  int textw, texth;
  Position pos;
  switch (self->type)
    {
    case DIGITAL_METER_TYPE_BPM:

      num_part =
        (int)
        tempo_track_get_current_bpm (P_TEMPO_TRACK);
      dec_part =
        (int)
        (tempo_track_get_current_bpm (
           P_TEMPO_TRACK) * 100) % 100;

      z_cairo_get_text_extents_for_widget (
        widget, self->seg7_layout, "88888",
        &textw, &texth);
      self->num_part_start_pos =
        ((width / 2) - textw / 2) -
        HALF_SPACE_BETWEEN;
      z_cairo_get_text_extents_for_widget (
        widget, self->seg7_layout, "888",
        &textw, &texth);
      self->num_part_end_pos =
        self->num_part_start_pos + textw;
      self->dec_part_start_pos =
        self->num_part_end_pos + SPACE_BETWEEN;
      z_cairo_get_text_extents_for_widget (
        widget, self->seg7_layout, "88",
        &textw, &texth);
      self->dec_part_end_pos =
        self->dec_part_start_pos + textw;
      self->height_start_pos =
        PADDING_TOP +
        caption_texth + HALF_SPACE_BETWEEN;
      self->height_end_pos =
        self->height_start_pos + texth;

      /* draw integer part */
      if (num_part < 100)
        sprintf (text, "!%d.", num_part);
      else
        sprintf (text, "%d.", num_part);
      pango_layout_set_markup (
        self->seg7_layout, text, -1);
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (
          self->num_part_start_pos,
          self->height_start_pos));
      gtk_snapshot_append_layout (
        snapshot, self->seg7_layout, &color);
      gtk_snapshot_restore (snapshot);

      /* draw decimal part */
      if (dec_part < 10)
        sprintf (text, "0%d", dec_part);
      else
        sprintf (text, "%d", dec_part);
      pango_layout_set_markup (
        self->seg7_layout, text, -1);
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (
          self->dec_part_start_pos,
          self->height_start_pos));
      gtk_snapshot_append_layout (
        snapshot, self->seg7_layout, &color);
      gtk_snapshot_restore (snapshot);

      break;
    case DIGITAL_METER_TYPE_POSITION:

      GET_POS;
      if (DISPLAY_TIME)
        {
          long ms = position_to_ms (&pos);
          long secs = ms / 1000;
          int mins = (int) secs / 60;
          ms = ms % 1000;
          secs = secs % 60;

          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout,
            /* MM:SS:ms 1 for each digit */
            "888888888", &textw, &texth);
          self->minutes_start_pos =
            ((width / 2) - textw / 2) -
            HALF_SPACE_BETWEEN * 3;
          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout, "88",
            &textw, &texth);
          self->minutes_end_pos =
            self->minutes_start_pos + textw;
          self->seconds_start_pos =
            self->minutes_end_pos + SPACE_BETWEEN;
          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout, "88",
            &textw, &texth);
          self->seconds_end_pos =
            self->seconds_start_pos + textw;
          self->ms_start_pos =
            self->seconds_end_pos + SPACE_BETWEEN;
          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout, "888",
            &textw, &texth);
          self->ms_end_pos =
            self->ms_start_pos + textw;

          self->height_start_pos =
            PADDING_TOP +
            caption_texth + HALF_SPACE_BETWEEN;
          self->height_end_pos =
            self->height_start_pos + texth;

          /* draw minutes */
          if (mins < 10)
            sprintf (text, "!%d.", mins);
          else
            sprintf (text, "%d.", mins);
          pango_layout_set_markup (
            self->seg7_layout, text, -1);
          gtk_snapshot_save (snapshot);
          gtk_snapshot_translate (
            snapshot,
            &GRAPHENE_POINT_INIT (
              self->minutes_start_pos,
              self->height_start_pos));
          gtk_snapshot_append_layout (
            snapshot, self->seg7_layout, &color);
          gtk_snapshot_restore (snapshot);

          /* draw seconds */
          if (secs < 10)
            sprintf (text, "0%ld.", secs);
          else
            sprintf (text, "%ld.", secs);
          pango_layout_set_markup (
            self->seg7_layout, text, -1);
          gtk_snapshot_save (snapshot);
          gtk_snapshot_translate (
            snapshot,
            &GRAPHENE_POINT_INIT (
              self->seconds_start_pos,
              self->height_start_pos));
          gtk_snapshot_append_layout (
            snapshot, self->seg7_layout, &color);
          gtk_snapshot_restore (snapshot);

          /* draw ms */
          if (ms < 10)
            sprintf (text, "00%ld", ms);
          else if (ms < 100)
            sprintf (text, "0%ld", ms);
          else
            sprintf (text, "%ld", ms);
          pango_layout_set_markup (
            self->seg7_layout, text, -1);
          gtk_snapshot_save (snapshot);
          gtk_snapshot_translate (
            snapshot,
            &GRAPHENE_POINT_INIT (
              self->ms_start_pos,
              self->height_start_pos));
          gtk_snapshot_append_layout (
            snapshot, self->seg7_layout, &color);
          gtk_snapshot_restore (snapshot);
        }
      else
        {
          bars = position_get_bars (&pos, true);
          beats = position_get_beats (&pos, true);
          sixteenths =
            position_get_sixteenths (&pos, true);
          ticks =
            (int) floor (position_get_ticks (&pos));

          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout,
            "-8888888888", &textw, &texth);
          self->bars_start_pos =
            ((width / 2) - textw / 2) -
            HALF_SPACE_BETWEEN * 3;
          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout, "-888",
            &textw, &texth);
          self->bars_end_pos =
            self->bars_start_pos + textw;
          self->beats_start_pos =
            self->bars_end_pos + SPACE_BETWEEN;
          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout, "8",
            &textw, &texth);
          self->beats_end_pos =
            self->beats_start_pos + textw;
          self->sixteenths_start_pos =
            self->beats_end_pos + SPACE_BETWEEN;
          self->sixteenths_end_pos =
            self->sixteenths_start_pos + textw;
          self->ticks_start_pos =
            self->sixteenths_end_pos + SPACE_BETWEEN;
          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout, "888",
            &textw, &texth);
          self->ticks_end_pos =
            self->ticks_start_pos + textw;
          self->height_start_pos =
            PADDING_TOP +
            caption_texth + HALF_SPACE_BETWEEN;
          self->height_end_pos =
            self->height_start_pos + texth;

          if (bars < -100)
            sprintf (text, "%d", bars);
          else if (bars < -10)
            sprintf (text, "!%d", bars);
          else if (bars < 0)
            sprintf (text, "!!%d", bars);
          else if (bars < 10)
            sprintf (text, "!!!%d", bars);
          else if (bars < 100)
            sprintf (text, "!!%d", bars);
          else
            sprintf (text, "!%d", bars);
          strcat (text, ".");
          pango_layout_set_markup (
            self->seg7_layout, text, -1);
          gtk_snapshot_save (snapshot);
          gtk_snapshot_translate (
            snapshot,
            &GRAPHENE_POINT_INIT (
              self->bars_start_pos,
              self->height_start_pos));
          gtk_snapshot_append_layout (
            snapshot, self->seg7_layout, &color);
          gtk_snapshot_restore (snapshot);

          sprintf (text, "%d.", abs (beats));
          pango_layout_set_markup (
            self->seg7_layout, text, -1);
          gtk_snapshot_save (snapshot);
          gtk_snapshot_translate (
            snapshot,
            &GRAPHENE_POINT_INIT (
              self->beats_start_pos,
              self->height_start_pos));
          gtk_snapshot_append_layout (
            snapshot, self->seg7_layout, &color);
          gtk_snapshot_restore (snapshot);

          sprintf (text, "%d.", abs (sixteenths));
          pango_layout_set_markup (
            self->seg7_layout, text, -1);
          gtk_snapshot_save (snapshot);
          gtk_snapshot_translate (
            snapshot,
            &GRAPHENE_POINT_INIT (
              self->sixteenths_start_pos,
              self->height_start_pos));
          gtk_snapshot_append_layout (
            snapshot, self->seg7_layout, &color);
          gtk_snapshot_restore (snapshot);

          if (abs (ticks) < 10)
            sprintf (text, "00%d", abs (ticks));
          else if (abs (ticks) < 100)
            sprintf (text, "0%d", abs (ticks));
          else
            sprintf (text, "%d", abs (ticks));
          pango_layout_set_markup (
            self->seg7_layout, text, -1);
          gtk_snapshot_save (snapshot);
          gtk_snapshot_translate (
            snapshot,
            &GRAPHENE_POINT_INIT (
              self->ticks_start_pos,
              self->height_start_pos));
          gtk_snapshot_append_layout (
            snapshot, self->seg7_layout, &color);
          gtk_snapshot_restore (snapshot);
        }
      break;
    case DIGITAL_METER_TYPE_NOTE_LENGTH:
      heap_text =
        snap_grid_stringize_length_and_type (
          *self->note_length,
          *self->note_type);
      z_cairo_get_text_extents_for_widget (
        widget, self->seg7_layout, heap_text,
        &textw, &texth);
      self->height_start_pos =
        PADDING_TOP +
        caption_texth + HALF_SPACE_BETWEEN;
      self->height_end_pos =
        self->height_start_pos + texth;
      pango_layout_set_markup (
        self->seg7_layout, heap_text, -1);
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (
          width / 2 - textw / 2,
          self->height_start_pos));
      gtk_snapshot_append_layout (
        snapshot, self->seg7_layout, &color);
      gtk_snapshot_restore (snapshot);
      g_free (heap_text);

      break;
    case DIGITAL_METER_TYPE_NOTE_TYPE:
      switch (*self->note_type)
        {
        case NOTE_TYPE_NORMAL:
          heap_text = _("normal");
          break;
        case NOTE_TYPE_DOTTED:
          heap_text = _("dotted");
          break;
        case NOTE_TYPE_TRIPLET:
          heap_text = _("triplet");
          break;
        }
      z_cairo_get_text_extents_for_widget (
        widget, self->seg7_layout, heap_text,
        &textw, &texth);
      self->height_start_pos =
        PADDING_TOP +
        caption_texth + HALF_SPACE_BETWEEN;
      self->height_end_pos =
        self->height_start_pos + texth;
      pango_layout_set_markup (
        self->seg7_layout, heap_text, -1);
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (
          width / 2 - textw / 2,
          self->height_start_pos));
      gtk_snapshot_append_layout (
        snapshot, self->seg7_layout, &color);
      gtk_snapshot_restore (snapshot);

      break;
    case DIGITAL_METER_TYPE_TIMESIG:

      z_cairo_get_text_extents_for_widget (
        widget, self->seg7_layout, "16/16",
        &textw, &texth);
      self->height_start_pos =
        PADDING_TOP +
        caption_texth + HALF_SPACE_BETWEEN;
      self->height_end_pos =
        self->height_start_pos + texth;

      BeatUnit bu =
        tempo_track_get_beat_unit_enum (
          P_TEMPO_TRACK);
      const char * beat_unit =
        beat_unit_strings[bu].str;
      int beats_per_bar =
        tempo_track_get_beats_per_bar (
          P_TEMPO_TRACK);
      if (beats_per_bar < 10)
        {
          text[0] = ' ';
          text[1] =
            (char) (beats_per_bar + '0');
        }
      else
        {
          text[0] =
            (char) ((beats_per_bar / 10) + '0');
          text[1] =
            (char) ((beats_per_bar % 10) + '0');
        }
      text[2] = '\0';
      heap_text =
        g_strdup_printf ("%s/%s", text, beat_unit);
      pango_layout_set_markup (
        self->seg7_layout, heap_text, -1);
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (
          width / 2 - textw / 2,
          self->height_start_pos));
      gtk_snapshot_append_layout (
        snapshot, self->seg7_layout, &color);
      gtk_snapshot_restore (snapshot);
      g_free (heap_text);

      break;
    }
}

/**
 * Updates the flags to know what to update when
 * scrolling/dragging.
 */
static void
update_flags (
  DigitalMeterWidget * self,
  double               x,
  double               y)
{
  int width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (self));
  switch (self->type)
    {
    case DIGITAL_METER_TYPE_BPM:
      if (y >= self->height_start_pos &&
          y <= self->height_end_pos)
        {
          if (x >= self->num_part_start_pos &&
              x <= self->num_part_end_pos)
            {
              self->update_num = 1;
            }
          else if (x >= self->dec_part_start_pos &&
                   x <= self->dec_part_end_pos)
            {
              self->update_dec = 1;
            }
        }
      break;

    case DIGITAL_METER_TYPE_POSITION:
      if (y >= self->height_start_pos &&
          y <= self->height_end_pos)
        {
          if (DISPLAY_TIME)
            {
              if (x >= self->minutes_start_pos &&
                  x <= self->minutes_end_pos)
                {
                  self->update_minutes = 1;
                }
              else if (x >= self->seconds_start_pos &&
                       x <= self->seconds_end_pos)
                {
                  self->update_seconds = 1;
                }
              else if (x >= self->ms_start_pos &&
                       x <= self->ms_end_pos)
                {
                  self->update_ms = 1;
                }
            }
          else
            {
              if (x >= self->bars_start_pos &&
                  x <= self->bars_end_pos)
                {
                  self->update_bars = 1;
                }
              else if (x >= self->beats_start_pos &&
                       x <= self->beats_end_pos)
                {
                  self->update_beats = 1;
                }
              else if (x >= self->sixteenths_start_pos &&
                       x <= self->sixteenths_end_pos)
                {
                  self->update_sixteenths = 1;
                }
              else if (x >= self->ticks_start_pos &&
                       x <= self->ticks_end_pos)
                {
                  self->update_ticks = 1;
                }
            }
        }

      break;
    case DIGITAL_METER_TYPE_NOTE_LENGTH:
      self->update_note_length = 1;
      break;
    case DIGITAL_METER_TYPE_NOTE_TYPE:
      self->update_note_type = 1;
      break;
    case DIGITAL_METER_TYPE_TIMESIG:
      if (x <= width / 2)
        {
          self->update_timesig_top = 1;
        }
      else
        {
          self->update_timesig_bot = 1;
        }
      break;
    }
}

/**
 * To be called when a change has started (eg
 * drag or scroll).
 */
static void
on_change_started (
  DigitalMeterWidget * self)
{
  switch (self->type)
    {
    case DIGITAL_METER_TYPE_NOTE_LENGTH:
      self->start_note_length =
        (NoteLength)
        *self->note_length;
      break;
    case DIGITAL_METER_TYPE_NOTE_TYPE:
      self->start_note_type =
        (NoteType)
        *self->note_type;
      break;
    case DIGITAL_METER_TYPE_TIMESIG:
      self->beats_per_bar_at_start =
        tempo_track_get_beats_per_bar (
          P_TEMPO_TRACK);
      self->beat_unit_at_start =
        tempo_track_get_beat_unit (P_TEMPO_TRACK);
      break;
    case DIGITAL_METER_TYPE_POSITION:
      if (self->on_drag_begin)
        ((*self->on_drag_begin) (self->obj));
      break;
    case DIGITAL_METER_TYPE_BPM:
      self->bpm_at_start =
        tempo_track_get_current_bpm (P_TEMPO_TRACK);
      self->last_set_bpm = self->bpm_at_start;
      transport_prepare_audio_regions_for_stretch (
        TRANSPORT, NULL);
      break;
    default:
      break;
    }
}

/**
 * To be called when a change has completed (eg
 * drag or scroll).
 */
static void
on_change_finished (
  DigitalMeterWidget * self)
{
  self->last_x = 0;
  self->last_y = 0;
  self->update_num = 0;
  self->update_dec = 0;
  self->update_bars = 0;
  self->update_beats = 0;
  self->update_sixteenths = 0;
  self->update_ticks = 0;
  self->update_minutes = 0;
  self->update_seconds = 0;
  self->update_ms = 0;
  /* FIXME super redundant */
  if (self->update_note_length ||
      self->update_note_type)
    {
      quantize_options_update_quantize_points (
        QUANTIZE_OPTIONS_TIMELINE);
      quantize_options_update_quantize_points (
        QUANTIZE_OPTIONS_EDITOR);
    }
  self->update_note_length = 0;
  self->update_note_type = 0;
  self->update_timesig_top = 0;
  self->update_timesig_bot = 0;

  switch (self->type)
    {
    case DIGITAL_METER_TYPE_POSITION:
      if (self->on_drag_end)
        ((*self->on_drag_end) (self->obj));
      break;
    case DIGITAL_METER_TYPE_BPM:
      tempo_track_set_bpm (
        P_TEMPO_TRACK,
        self->last_set_bpm, self->bpm_at_start,
        Z_F_NOT_TEMPORARY, F_PUBLISH_EVENTS);
      break;
    case DIGITAL_METER_TYPE_TIMESIG:
      {
        /* no update if rolling */
        if (TRANSPORT_IS_ROLLING)
          {
            break;
          }

        int beats_per_bar =
          tempo_track_get_beats_per_bar (
            P_TEMPO_TRACK);
        int beat_unit =
          tempo_track_get_beat_unit (
            P_TEMPO_TRACK);
        if (self->beats_per_bar_at_start !=
              beats_per_bar)
          {
            GError * err = NULL;
            bool ret =
              transport_action_perform_time_sig_change (
                TRANSPORT_ACTION_BEATS_PER_BAR_CHANGE,
                self->beats_per_bar_at_start,
                beats_per_bar,
                F_ALREADY_EDITED, &err);
            if (!ret)
              {
                HANDLE_ERROR (
                  err, "%s",
                  _("Failed to change time "
                  "signature"));
              }
          }
        else if (self->beat_unit_at_start !=
                   beat_unit)
          {
            GError * err = NULL;
            bool ret =
              transport_action_perform_time_sig_change (
                TRANSPORT_ACTION_BEAT_UNIT_CHANGE,
                self->beat_unit_at_start, beat_unit,
                F_ALREADY_EDITED, &err);
            if (!ret)
              {
                HANDLE_ERROR (
                  err, "%s",
                  _("Failed to change time "
                  "signature"));
              }
          }
      }
    break;
    default:
      break;
    }
}

void
digital_meter_set_draw_line (
  DigitalMeterWidget * self,
  int                  draw_line)
{
  self->draw_line = draw_line;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * Motion callback.
 */
static void
on_motion (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  gpointer                   user_data)
{
  DigitalMeterWidget * self =
    Z_DIGITAL_METER_WIDGET (user_data);

  self->hover_x = MAX (x, 0.0);
  self->hover_y = MAX (y, 0.0);
}

static gboolean
on_scroll (
  GtkEventControllerScroll * scroll_controller,
  gdouble                    dx,
  gdouble                    dy,
  gpointer                   user_data)
{
  DigitalMeterWidget * self =
    Z_DIGITAL_METER_WIDGET (user_data);

  GdkEvent * event =
    gtk_event_controller_get_current_event (
      GTK_EVENT_CONTROLLER (scroll_controller));
  GdkScrollDirection direction =
    gdk_scroll_event_get_direction (event);

  int num =
    (direction == GDK_SCROLL_UP
     || direction == GDK_SCROLL_RIGHT) ? 1 : -1;

  update_flags (self, self->hover_x, self->hover_y);
  on_change_started (self);

  ControlPortChange change = { 0 };
  Position pos;
  switch (self->type)
    {
    case DIGITAL_METER_TYPE_BPM:
      change.flag1 = PORT_FLAG_BPM;
      if (self->update_num)
        {
          change.real_val =
            self->last_set_bpm + (bpm_t) num;
          self->last_set_bpm = change.real_val;
          router_queue_control_port_change (
            ROUTER, &change);
        }
      else if (self->update_dec)
        {
          change.real_val =
            self->last_set_bpm +
            (bpm_t) num / 100.f;
          self->last_set_bpm = change.real_val;
          router_queue_control_port_change (
            ROUTER, &change);
        }

      break;

    case DIGITAL_METER_TYPE_POSITION:
      GET_POS;
      if (DISPLAY_TIME)
        {
          long ms = 0;
          if (self->update_minutes)
            {
              ms = num * 60 * 1000;
            }
          else if (self->update_seconds)
            {
              ms = num * 1000;
            }
          else if (self->update_ms)
            {
              ms = num;
            }
          position_add_ms (&pos, ms);
        }
      else
        {
          if (self->update_bars)
            {
              position_add_bars (&pos, num);
            }
          else if (self->update_beats)
            {
              position_add_beats (&pos, num);
            }
          else if (self->update_sixteenths)
            {
              position_add_sixteenths (&pos, num);
            }
          else if (self->update_ticks)
            {
              position_add_ticks (&pos, num);
            }
          SET_POS;
        }
      break;
    case DIGITAL_METER_TYPE_NOTE_LENGTH:
      if (self->update_note_length)
        {
          num += self->start_note_length;
          if (num < 0)
            *self->note_length = 0;
          else
            *self->note_length =
              (NoteLength)
              (num > NOTE_LENGTH_1_128
               ? NOTE_LENGTH_1_128 : num);
        }
      break;
    case DIGITAL_METER_TYPE_NOTE_TYPE:
      if (self->update_note_type)
        {
          num += self->start_note_type;
          if (num < 0)
            *self->note_type = 0;
          else
            *self->note_type =
              (NoteType)
              (num > NOTE_TYPE_TRIPLET
               ? NOTE_TYPE_TRIPLET : num);
        }
      break;
    case DIGITAL_METER_TYPE_TIMESIG:
      /* no update if rolling */
      if (TRANSPORT_IS_ROLLING)
        {
          break;
        }
      if (self->update_timesig_top)
        {
          num += self->beats_per_bar_at_start;
          change.flag2 =
            PORT_FLAG2_BEATS_PER_BAR;
          if (num < TEMPO_TRACK_MIN_BEATS_PER_BAR)
            {
              change.ival =
                TEMPO_TRACK_MIN_BEATS_PER_BAR;
            }
          else
            {
              change.ival =
                num > TEMPO_TRACK_MAX_BEATS_PER_BAR
                ? TEMPO_TRACK_MAX_BEATS_PER_BAR
                : num;
            }
          router_queue_control_port_change (
            ROUTER, &change);
        }
      else if (self->update_timesig_bot)
        {
          num +=
            (int)
            tempo_track_beat_unit_to_enum (
              self->beat_unit_at_start);
          change.flag2 = PORT_FLAG2_BEAT_UNIT;
          if (num < 0)
            {
              change.beat_unit = BEAT_UNIT_2;
            }
          else
            {
              change.beat_unit =
                num > BEAT_UNIT_16
                ? BEAT_UNIT_16 : (BeatUnit) num;
            }
          router_queue_control_port_change (
            ROUTER, &change);
        }
      if (self->update_timesig_top ||
          self->update_timesig_bot)
        {
          EVENTS_PUSH (
            ET_TIME_SIGNATURE_CHANGED, NULL);
        }
      break;
    }
  on_change_finished (self);
  gtk_widget_queue_draw (GTK_WIDGET (self));

  return true;
}

static void
drag_begin (
  GtkGestureDrag * gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  DigitalMeterWidget * self)
{
  on_change_started (self);
}

static void
drag_update (
  GtkGestureDrag *     gesture,
  gdouble              offset_x,
  gdouble              offset_y,
  DigitalMeterWidget * self)
{
  offset_y = - offset_y;
  int use_x = fabs (offset_x) > fabs (offset_y);
  double diff =
    use_x ?
    offset_x - self->last_x :
    offset_y - self->last_y;
  int num;
  float dec;
  Position pos;
  switch (self->type)
    {
    case DIGITAL_METER_TYPE_BPM:
      /*g_message ("update num ? %d", self->update_num);*/
      if (self->update_num)
        {
          num = (int) diff / 4;
          /*g_message ("updating num with %d", num);*/
          if (abs (num) > 0)
            {
              ControlPortChange change = { 0 };
              change.flag1 = PORT_FLAG_BPM;
              change.real_val =
                self->last_set_bpm + (bpm_t) num;
              self->last_set_bpm = change.real_val;
              router_queue_control_port_change (
                ROUTER, &change);
              self->last_y = offset_y;
              self->last_x = offset_x;
            }
        }
      else if (self->update_dec)
        {
          dec = (float) diff / 400.f;
          g_message ("%f", (double) dec);
          if (fabs (dec) > 0)
            {
              ControlPortChange change = { 0 };
              change.flag1 = PORT_FLAG_BPM;
              change.real_val =
                self->last_set_bpm + (bpm_t) dec;
              self->last_set_bpm = change.real_val;
              router_queue_control_port_change (
                ROUTER, &change);
              self->last_y = offset_y;
              self->last_x = offset_x;
            }

        }

      break;

    case DIGITAL_METER_TYPE_POSITION:
      GET_POS;
      if (DISPLAY_TIME)
        {
          if (self->update_minutes)
            {
              g_message ("UPDATE MINS");
              num = (int) diff / 4;
              if (abs (num) > 0)
                {
              g_message ("UPDATE MINS %d", num);
                  position_print (&pos);
                  position_add_minutes (
                    &pos, num);
                  position_print (&pos);
                  self->last_y = offset_y;
                  self->last_x = offset_x;
                }
            }
          else if (self->update_seconds)
            {
              num = (int) diff / 4;
              if (abs (num) > 0)
                {
                  position_add_seconds (
                    &pos, num);
                  self->last_y = offset_y;
                  self->last_x = offset_x;
                }
            }
          else if (self->update_ms)
            {
              num = (int) diff / 4;
              if (abs (num) > 0)
                {
                  position_add_ms (
                    &pos, num);
                  self->last_y = offset_y;
                  self->last_x = offset_x;
                }
            }
        }
      else
        {
          if (self->update_bars)
            {
              num = (int) diff / 4;
              if (abs (num) > 0)
                {
                  position_add_bars (&pos, num);
                  self->last_y = offset_y;
                  self->last_x = offset_x;
                }
            }
          else if (self->update_beats)
            {
              num = (int) diff / 4;
              if (abs (num) > 0)
                {
                  position_add_beats (&pos, num);
                  self->last_y = offset_y;
                  self->last_x = offset_x;
                }
            }
          else if (self->update_sixteenths)
            {
              num = (int) diff / 4;
              if (abs (num) > 0)
                {
                  position_add_sixteenths (
                    &pos, num);
                  self->last_y = offset_y;
                  self->last_x = offset_x;
                }
            }
          else if (self->update_ticks)
            {
              num = (int) diff / 4;
              if (abs (num) > 0)
                {
                  position_add_ticks (&pos, num);
                  self->last_y = offset_y;
                  self->last_x = offset_x;
                }
            }
        }
        SET_POS;
      break;
    case DIGITAL_METER_TYPE_NOTE_LENGTH:
      if (self->update_note_length)
        {
          num = self->start_note_length + (int) offset_y / 24;
          if (num < 0)
            *self->note_length = 0;
          else
            *self->note_length =
              (NoteLength)
              (num > NOTE_LENGTH_1_128
               ? NOTE_LENGTH_1_128 : num);
        }
      break;
    case DIGITAL_METER_TYPE_NOTE_TYPE:
      if (self->update_note_type)
        {
          num = self->start_note_type + (int) offset_y / 24;
          if (num < 0)
            *self->note_type = 0;
          else
            *self->note_type =
              num > NOTE_TYPE_TRIPLET
              ? NOTE_TYPE_TRIPLET : (NoteType) num;
        }
      break;
    case DIGITAL_METER_TYPE_TIMESIG:
      /* no update if rolling */
      if (TRANSPORT_IS_ROLLING)
        {
          break;
        }
      if (self->update_timesig_top)
        {
          num =
            self->beats_per_bar_at_start +
            (int) diff / 24;
          ControlPortChange change = { 0 };
          change.flag2 =
            PORT_FLAG2_BEATS_PER_BAR;
          if (num < TEMPO_TRACK_MIN_BEATS_PER_BAR)
            {
              change.ival =
                TEMPO_TRACK_MIN_BEATS_PER_BAR;
            }
          else
            {
              change.ival =
                num > TEMPO_TRACK_MAX_BEATS_PER_BAR
                ? TEMPO_TRACK_MAX_BEATS_PER_BAR
                : num;
            }
          router_queue_control_port_change (
            ROUTER, &change);
        }
      else if (self->update_timesig_bot)
        {
          num =
            (int)
            tempo_track_beat_unit_to_enum (
              self->beat_unit_at_start) +
            (int) diff / 24;
          ControlPortChange change = { 0 };
          change.flag2 = PORT_FLAG2_BEAT_UNIT;
          if (num < 0)
            {
              change.beat_unit = BEAT_UNIT_2;
            }
          else
            {
              change.beat_unit =
                num > BEAT_UNIT_16
                ? BEAT_UNIT_16 : (BeatUnit) num;
            }
          router_queue_control_port_change (
            ROUTER, &change);
        }
      if (self->update_timesig_top ||
          self->update_timesig_bot)
        {
          EVENTS_PUSH (
            ET_TIME_SIGNATURE_CHANGED, NULL);
        }
      break;
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
drag_end (
  GtkGestureDrag *gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  DigitalMeterWidget * self)
{
  on_change_finished (self);
}

static void
button_press_cb (
  GtkGestureClick *    gesture,
  gint                 n_press,
  gdouble              x,
  gdouble              y,
  DigitalMeterWidget * self)
{
  /*g_message ("%d, %d", self->height_start_pos, self->height_end_pos);*/
  update_flags (self, x, y);
}

/**
 * Creates a digital meter with the given type (bpm or position).
 */
DigitalMeterWidget *
digital_meter_widget_new (
  DigitalMeterType  type,
  NoteLength *      note_length,
  NoteType *        note_type,
  const char *      caption)
{
  DigitalMeterWidget * self =
    g_object_new (DIGITAL_METER_WIDGET_TYPE, NULL);

  self->type = type;
  self->caption = g_strdup (caption);
  self->note_length = note_length;
  self->note_type = note_type;

  return self;
}

/**
 * Creates a digital meter for an arbitrary position.
 *
 * @param obj The object to call the get/setters with.
 *
 *   E.g. Region.
 * @param get_val The getter func to get the position,
 *   passing the obj and the position to save to.
 * @param set_val The setter function to set the
 *   position.
 */
DigitalMeterWidget *
_digital_meter_widget_new_for_position (
  void * obj,
  void (*on_drag_begin)(void *),
  void (*get_val)(void *, Position *),
  void (*set_val)(void *, Position *),
  void (*on_drag_end)(void *),
  const char * caption)
{
  DigitalMeterWidget * self =
    g_object_new (DIGITAL_METER_WIDGET_TYPE, NULL);

  self->obj = obj;
  self->on_drag_begin = on_drag_begin;
  self->getter = get_val;
  self->setter = set_val;
  self->on_drag_end = on_drag_end;
  self->caption = g_strdup (caption);
  self->type = DIGITAL_METER_TYPE_POSITION;

  return self;
}

static void
finalize (
  DigitalMeterWidget * self)
{
  if (self->caption)
    g_free (self->caption);
  if (self->caption_layout)
    g_object_unref (self->caption_layout);
  if (self->seg7_layout)
    g_object_unref (self->seg7_layout);
  if (self->normal_layout)
    g_object_unref (self->normal_layout);

  G_OBJECT_CLASS (
    digital_meter_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
digital_meter_widget_class_init (
  DigitalMeterWidgetClass * klass)
{
  GtkWidgetClass * wklass =
    GTK_WIDGET_CLASS (klass);
  wklass->snapshot = digital_meter_snapshot;
  gtk_widget_class_set_css_name (
    wklass, "digital-meter");

  GObjectClass * oklass =
    G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) finalize;
}

static void
digital_meter_widget_init (
  DigitalMeterWidget * self)
{
  g_return_if_fail (
    Z_IS_DIGITAL_METER_WIDGET (self));

  self->drag =
    GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->drag));

  GtkEventControllerMotion * motion =
    GTK_EVENT_CONTROLLER_MOTION (
      gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion), "motion",
    G_CALLBACK (on_motion),  self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (motion));

  GtkEventControllerScroll * scroll_controller =
    GTK_EVENT_CONTROLLER_SCROLL (
      gtk_event_controller_scroll_new (
        GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES));
  g_signal_connect (
    G_OBJECT (scroll_controller), "scroll",
    G_CALLBACK (on_scroll),  self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (scroll_controller));

  GtkGestureClick * click_gesture =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  g_signal_connect (
    G_OBJECT (click_gesture), "pressed",
    G_CALLBACK (button_press_cb), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (click_gesture));
}
