// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>
#include <cstdlib>

#include "actions/transport_action.h"
#include "dsp/control_port.h"
#include "dsp/position.h"
#include "dsp/quantize_options.h"
#include "dsp/router.h"
#include "dsp/snap_grid.h"
#include "dsp/tempo_track.h"
#include "dsp/tracklist.h"
#include "dsp/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "utils/color.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (DigitalMeterWidget, digital_meter_widget, GTK_TYPE_WIDGET)

#define FONT_SIZE 16
/*#define SEG7_FONT "Segment7 Bold 16"*/
#define SEG7_FONT "DSEG14 Classic Mini Italic 12"
#define NORMAL_FONT "sans-serif Bold 11"
#define CAPTION_FONT "sans-serif 7"
#define SPACE_BETWEEN 6
#define HALF_SPACE_BETWEEN 2
#define PADDING_W 4
#define PADDING_TOP 0

static bool
display_time (DigitalMeterWidget * self)
{
  return self->is_transport
         && static_cast<Transport::Display> (
              g_settings_get_enum (S_UI, "transport-display"))
              == Transport::Display::Time;
}

#define SET_POS ((*self->setter) (self->obj, &pos))
#define GET_POS ((*self->getter) (self->obj, &pos))

static void
recreate_pango_layouts (DigitalMeterWidget * self)
{
  if (PANGO_IS_LAYOUT (self->caption_layout))
    g_object_unref (self->caption_layout);
  if (PANGO_IS_LAYOUT (self->seg7_layout))
    g_object_unref (self->seg7_layout);
  if (PANGO_IS_LAYOUT (self->normal_layout))
    g_object_unref (self->normal_layout);

  self->caption_layout =
    z_cairo_create_pango_layout_from_string (
      GTK_WIDGET (self), CAPTION_FONT, PANGO_ELLIPSIZE_NONE, -1)
      .release ();
  self->seg7_layout =
    z_cairo_create_pango_layout_from_string (
      GTK_WIDGET (self), SEG7_FONT, PANGO_ELLIPSIZE_NONE, -1)
      .release ();
  self->normal_layout =
    z_cairo_create_pango_layout_from_string (
      GTK_WIDGET (self), NORMAL_FONT, PANGO_ELLIPSIZE_NONE, -1)
      .release ();
}

static void
init_dm (DigitalMeterWidget * self)
{
  z_return_if_fail (Z_DIGITAL_METER_WIDGET (self));

  recreate_pango_layouts (self);

  int caption_textw, caption_texth;
  z_cairo_get_text_extents_for_widget (
    self, self->caption_layout, self->caption, &caption_textw, &caption_texth);
  int textw, texth;
  switch (self->type)
    {
    case DigitalMeterType::DIGITAL_METER_TYPE_BPM:
      {
        gtk_widget_set_tooltip_text ((GtkWidget *) self, _ ("Tempo/BPM"));
        z_cairo_get_text_extents_for_widget (
          self, self->seg7_layout, "888888", &textw, &texth);
        /* caption + padding between caption and
         * BPM + padding top/bottom */
        gtk_widget_set_size_request (
          GTK_WIDGET (self), textw + PADDING_W * 2,
          caption_texth + HALF_SPACE_BETWEEN + texth + PADDING_TOP * 2);
      }
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_POSITION:
      gtk_widget_set_tooltip_text ((GtkWidget *) self, _ ("Position"));
      z_cairo_get_text_extents_for_widget (
        self, self->seg7_layout, "-888888888", &textw, &texth);
      /* caption + padding between caption and
       * BPM + padding top/bottom */
      gtk_widget_set_size_request (
        GTK_WIDGET (self), textw + PADDING_W * 2 + HALF_SPACE_BETWEEN * 3,
        caption_texth + HALF_SPACE_BETWEEN + texth + PADDING_TOP * 2);

      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_NOTE_LENGTH:
      gtk_widget_set_size_request (GTK_WIDGET (self), -1, 30);
      break;

    case DigitalMeterType::DIGITAL_METER_TYPE_NOTE_TYPE:
      gtk_widget_set_size_request (GTK_WIDGET (self), -1, 30);
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_TIMESIG:
      {
        gtk_widget_set_tooltip_text (
          GTK_WIDGET (self),
          _ ("Time Signature - Beats per bar / "
             "Beat unit"));
        z_cairo_get_text_extents_for_widget (
          self, self->seg7_layout, "16/16", &textw, &texth);
        /* caption + padding between caption and
         * BPM + padding top/bottom */
        gtk_widget_set_size_request (
          GTK_WIDGET (self), textw + PADDING_W * 2,
          caption_texth + HALF_SPACE_BETWEEN + texth + PADDING_TOP * 2);
      }
      break;
    }

  self->initialized = true;
}

static void
digital_meter_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  DigitalMeterWidget * self = Z_DIGITAL_METER_WIDGET (widget);

  int width = gtk_widget_get_width (widget);

  if (!PROJECT->loaded_)
    return;

  if (!self->initialized)
    init_dm (self);

  /* draw caption and get its extents */
  int     caption_textw, caption_texth;
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
    self, self->caption_layout, self->caption, &caption_textw, &caption_texth);
  pango_layout_set_markup (self->caption_layout, self->caption, -1);
  gtk_snapshot_save (snapshot);
  {
    graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
      (float) width / 2.f - (float) caption_textw / 2.f, (float) PADDING_TOP);
    gtk_snapshot_translate (snapshot, &tmp_pt);
  }
  gtk_snapshot_append_layout (snapshot, self->caption_layout, &color);
  gtk_snapshot_restore (snapshot);

  /* draw line */
  if (self->draw_line)
    {
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          0.f, (float) caption_texth + (float) PADDING_TOP, (float) width, 1.f);
        gtk_snapshot_append_color (snapshot, &color, &tmp_r);
      }
    }

  /*GdkRGBA color;*/
  gdk_rgba_parse (&color, "#57e389");
  if (!gtk_widget_is_sensitive (GTK_WIDGET (self)))
    {
      color = Color (color).brighten_default ().to_gdk_rgba ();
    }
  char        text[20];
  std::string heap_text;
  int         num_part, dec_part, bars, beats, sixteenths, ticks;
  int         textw, texth;
  Position    pos;
  switch (self->type)
    {
    case DigitalMeterType::DIGITAL_METER_TYPE_BPM:

      num_part = (int) P_TEMPO_TRACK->get_current_bpm ();
      dec_part = (int) (P_TEMPO_TRACK->get_current_bpm () * 100) % 100;

      z_cairo_get_text_extents_for_widget (
        widget, self->seg7_layout, "88888", &textw, &texth);
      self->num_part_start_pos = ((width / 2) - textw / 2) - HALF_SPACE_BETWEEN;
      z_cairo_get_text_extents_for_widget (
        widget, self->seg7_layout, "888", &textw, &texth);
      self->num_part_end_pos = self->num_part_start_pos + textw;
      self->dec_part_start_pos = self->num_part_end_pos + SPACE_BETWEEN;
      z_cairo_get_text_extents_for_widget (
        widget, self->seg7_layout, "88", &textw, &texth);
      self->dec_part_end_pos = self->dec_part_start_pos + textw;
      self->height_start_pos = PADDING_TOP + caption_texth + HALF_SPACE_BETWEEN;
      self->height_end_pos = self->height_start_pos + texth;

      /* draw integer part */
      if (num_part < 100)
        sprintf (text, "!%d.", num_part);
      else
        sprintf (text, "%d.", num_part);
      pango_layout_set_markup (self->seg7_layout, text, -1);
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
          (float) self->num_part_start_pos, (float) self->height_start_pos);
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      gtk_snapshot_append_layout (snapshot, self->seg7_layout, &color);
      gtk_snapshot_restore (snapshot);

      /* draw decimal part */
      if (dec_part < 10)
        sprintf (text, "0%d", dec_part);
      else
        sprintf (text, "%d", dec_part);
      pango_layout_set_markup (self->seg7_layout, text, -1);
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
          (float) self->dec_part_start_pos, (float) self->height_start_pos);
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      gtk_snapshot_append_layout (snapshot, self->seg7_layout, &color);
      gtk_snapshot_restore (snapshot);

      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_POSITION:

      GET_POS;
      if (display_time (self))
        {
          long ms = pos.to_ms ();
          long secs = ms / 1000;
          int  mins = (int) secs / 60;
          ms = ms % 1000;
          secs = secs % 60;

          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout,
            /* MM:SS:ms 1 for each digit */
            "888888888", &textw, &texth);
          self->minutes_start_pos =
            ((width / 2) - textw / 2) - HALF_SPACE_BETWEEN * 3;
          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout, "88", &textw, &texth);
          self->minutes_end_pos = self->minutes_start_pos + textw;
          self->seconds_start_pos = self->minutes_end_pos + SPACE_BETWEEN;
          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout, "88", &textw, &texth);
          self->seconds_end_pos = self->seconds_start_pos + textw;
          self->ms_start_pos = self->seconds_end_pos + SPACE_BETWEEN;
          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout, "888", &textw, &texth);
          self->ms_end_pos = self->ms_start_pos + textw;

          self->height_start_pos =
            PADDING_TOP + caption_texth + HALF_SPACE_BETWEEN;
          self->height_end_pos = self->height_start_pos + texth;

          /* draw minutes */
          if (mins < 10)
            sprintf (text, "!%d.", mins);
          else
            sprintf (text, "%d.", mins);
          pango_layout_set_markup (self->seg7_layout, text, -1);
          gtk_snapshot_save (snapshot);
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
              (float) self->minutes_start_pos, (float) self->height_start_pos);
            gtk_snapshot_translate (snapshot, &tmp_pt);
          }
          gtk_snapshot_append_layout (snapshot, self->seg7_layout, &color);
          gtk_snapshot_restore (snapshot);

          /* draw seconds */
          if (secs < 10)
            sprintf (text, "0%ld.", secs);
          else
            sprintf (text, "%ld.", secs);
          pango_layout_set_markup (self->seg7_layout, text, -1);
          gtk_snapshot_save (snapshot);
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
              (float) self->seconds_start_pos, (float) self->height_start_pos);
            gtk_snapshot_translate (snapshot, &tmp_pt);
          }
          gtk_snapshot_append_layout (snapshot, self->seg7_layout, &color);
          gtk_snapshot_restore (snapshot);

          /* draw ms */
          if (ms < 10)
            sprintf (text, "00%ld", ms);
          else if (ms < 100)
            sprintf (text, "0%ld", ms);
          else
            sprintf (text, "%ld", ms);
          pango_layout_set_markup (self->seg7_layout, text, -1);
          gtk_snapshot_save (snapshot);
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
              (float) self->ms_start_pos, (float) self->height_start_pos);
            gtk_snapshot_translate (snapshot, &tmp_pt);
          }
          gtk_snapshot_append_layout (snapshot, self->seg7_layout, &color);
          gtk_snapshot_restore (snapshot);
        }
      else
        {
          bars = pos.get_bars (true);
          beats = pos.get_beats (true);
          sixteenths = pos.get_sixteenths (true);
          ticks = (int) floor (pos.get_ticks ());

          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout, "-8888888888", &textw, &texth);
          self->bars_start_pos =
            ((width / 2) - textw / 2) - HALF_SPACE_BETWEEN * 3;
          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout, "-888", &textw, &texth);
          self->bars_end_pos = self->bars_start_pos + textw;
          self->beats_start_pos = self->bars_end_pos + SPACE_BETWEEN;
          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout, "8", &textw, &texth);
          self->beats_end_pos = self->beats_start_pos + textw;
          self->sixteenths_start_pos = self->beats_end_pos + SPACE_BETWEEN;
          self->sixteenths_end_pos = self->sixteenths_start_pos + textw;
          self->ticks_start_pos = self->sixteenths_end_pos + SPACE_BETWEEN;
          z_cairo_get_text_extents_for_widget (
            widget, self->seg7_layout, "888", &textw, &texth);
          self->ticks_end_pos = self->ticks_start_pos + textw;
          self->height_start_pos =
            PADDING_TOP + caption_texth + HALF_SPACE_BETWEEN;
          self->height_end_pos = self->height_start_pos + texth;

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
          pango_layout_set_markup (self->seg7_layout, text, -1);
          gtk_snapshot_save (snapshot);
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
              (float) self->bars_start_pos, (float) self->height_start_pos);
            gtk_snapshot_translate (snapshot, &tmp_pt);
          }
          gtk_snapshot_append_layout (snapshot, self->seg7_layout, &color);
          gtk_snapshot_restore (snapshot);

          sprintf (text, "%d.", abs (beats));
          pango_layout_set_markup (self->seg7_layout, text, -1);
          gtk_snapshot_save (snapshot);
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
              (float) self->beats_start_pos, (float) self->height_start_pos);
            gtk_snapshot_translate (snapshot, &tmp_pt);
          }
          gtk_snapshot_append_layout (snapshot, self->seg7_layout, &color);
          gtk_snapshot_restore (snapshot);

          sprintf (text, "%d.", abs (sixteenths));
          pango_layout_set_markup (self->seg7_layout, text, -1);
          gtk_snapshot_save (snapshot);
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
              (float) self->sixteenths_start_pos,
              (float) self->height_start_pos);
            gtk_snapshot_translate (snapshot, &tmp_pt);
          }
          gtk_snapshot_append_layout (snapshot, self->seg7_layout, &color);
          gtk_snapshot_restore (snapshot);

          if (abs (ticks) < 10)
            sprintf (text, "00%d", abs (ticks));
          else if (abs (ticks) < 100)
            sprintf (text, "0%d", abs (ticks));
          else
            sprintf (text, "%d", abs (ticks));
          pango_layout_set_markup (self->seg7_layout, text, -1);
          gtk_snapshot_save (snapshot);
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
              (float) self->ticks_start_pos, (float) self->height_start_pos);
            gtk_snapshot_translate (snapshot, &tmp_pt);
          }
          gtk_snapshot_append_layout (snapshot, self->seg7_layout, &color);
          gtk_snapshot_restore (snapshot);
        }
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_NOTE_LENGTH:
      heap_text = SnapGrid::stringize_length_and_type (
        *self->note_length, *self->note_type);
      z_cairo_get_text_extents_for_widget (
        widget, self->seg7_layout, heap_text.c_str (), &textw, &texth);
      self->height_start_pos = PADDING_TOP + caption_texth + HALF_SPACE_BETWEEN;
      self->height_end_pos = self->height_start_pos + texth;
      pango_layout_set_markup (self->seg7_layout, heap_text.c_str (), -1);
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
          (float) width / 2.f - (float) textw / 2.f,
          (float) self->height_start_pos);
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      gtk_snapshot_append_layout (snapshot, self->seg7_layout, &color);
      gtk_snapshot_restore (snapshot);

      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_NOTE_TYPE:
      switch (*self->note_type)
        {
        case NoteType::NOTE_TYPE_NORMAL:
          heap_text = _ ("normal");
          break;
        case NoteType::NOTE_TYPE_DOTTED:
          heap_text = _ ("dotted");
          break;
        case NoteType::NOTE_TYPE_TRIPLET:
          heap_text = _ ("triplet");
          break;
        }
      z_cairo_get_text_extents_for_widget (
        widget, self->seg7_layout, heap_text.c_str (), &textw, &texth);
      self->height_start_pos = PADDING_TOP + caption_texth + HALF_SPACE_BETWEEN;
      self->height_end_pos = self->height_start_pos + texth;
      pango_layout_set_markup (self->seg7_layout, heap_text.c_str (), -1);
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
          (float) width / 2.f - (float) textw / 2.f,
          (float) self->height_start_pos);
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      gtk_snapshot_append_layout (snapshot, self->seg7_layout, &color);
      gtk_snapshot_restore (snapshot);

      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_TIMESIG:

      z_cairo_get_text_extents_for_widget (
        widget, self->seg7_layout, "16/16", &textw, &texth);
      self->height_start_pos = PADDING_TOP + caption_texth + HALF_SPACE_BETWEEN;
      self->height_end_pos = self->height_start_pos + texth;

      BeatUnit bu = P_TEMPO_TRACK->get_beat_unit_enum ();
      int      bu_int = TempoTrack::beat_unit_enum_to_int (bu);
      int      beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
      if (beats_per_bar < 10)
        {
          text[0] = ' ';
          text[1] = (char) (beats_per_bar + '0');
        }
      else
        {
          text[0] = (char) ((beats_per_bar / 10) + '0');
          text[1] = (char) ((beats_per_bar % 10) + '0');
        }
      text[2] = '\0';
      heap_text = fmt::format ("{}/{}", text, bu_int);
      pango_layout_set_markup (self->seg7_layout, heap_text.c_str (), -1);
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
          (float) width / 2.f - (float) textw / 2.f,
          (float) self->height_start_pos);
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      gtk_snapshot_append_layout (snapshot, self->seg7_layout, &color);
      gtk_snapshot_restore (snapshot);

      break;
    }
}

/**
 * Updates the flags to know what to update when
 * scrolling/dragging.
 */
static void
update_flags (DigitalMeterWidget * self, double x, double y)
{
  z_debug ("update flags {:f} {:f}", x, y);
  int width = gtk_widget_get_width (GTK_WIDGET (self));
  switch (self->type)
    {
    case DigitalMeterType::DIGITAL_METER_TYPE_BPM:
      if (y >= self->height_start_pos && y <= self->height_end_pos)
        {
          if (x >= self->num_part_start_pos && x <= self->num_part_end_pos)
            {
              self->update_num = 1;
            }
          else if (x >= self->dec_part_start_pos && x <= self->dec_part_end_pos)
            {
              self->update_dec = 1;
            }
        }
      break;

    case DigitalMeterType::DIGITAL_METER_TYPE_POSITION:
      if (y >= self->height_start_pos && y <= self->height_end_pos)
        {
          if (display_time (self))
            {
              if (x >= self->minutes_start_pos && x <= self->minutes_end_pos)
                {
                  self->update_minutes = 1;
                }
              else if (
                x >= self->seconds_start_pos && x <= self->seconds_end_pos)
                {
                  self->update_seconds = 1;
                }
              else if (x >= self->ms_start_pos && x <= self->ms_end_pos)
                {
                  self->update_ms = 1;
                }
            }
          else
            {
              if (x >= self->bars_start_pos && x <= self->bars_end_pos)
                {
                  self->update_bars = 1;
                }
              else if (x >= self->beats_start_pos && x <= self->beats_end_pos)
                {
                  self->update_beats = 1;
                }
              else if (
                x >= self->sixteenths_start_pos && x <= self->sixteenths_end_pos)
                {
                  self->update_sixteenths = 1;
                }
              else if (x >= self->ticks_start_pos && x <= self->ticks_end_pos)
                {
                  self->update_ticks = 1;
                }
            }
        }

      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_NOTE_LENGTH:
      self->update_note_length = 1;
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_NOTE_TYPE:
      self->update_note_type = 1;
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_TIMESIG:
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
on_change_started (DigitalMeterWidget * self)
{
  z_debug ("change started");
  switch (self->type)
    {
    case DigitalMeterType::DIGITAL_METER_TYPE_NOTE_LENGTH:
      self->start_note_length = (int) *self->note_length;
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_NOTE_TYPE:
      self->start_note_type = (int) *self->note_type;
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_TIMESIG:
      self->beats_per_bar_at_start = P_TEMPO_TRACK->get_beats_per_bar ();
      self->beat_unit_at_start = P_TEMPO_TRACK->get_beat_unit ();
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_POSITION:
      if (self->on_drag_begin)
        ((*self->on_drag_begin) (self->obj));
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_BPM:
      self->bpm_at_start = P_TEMPO_TRACK->get_current_bpm ();
      self->last_set_bpm = self->bpm_at_start;
      TRANSPORT->prepare_audio_regions_for_stretch (nullptr);
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
on_change_finished (DigitalMeterWidget * self)
{
  z_debug ("change finished");
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
  if (self->update_note_length || self->update_note_type)
    {
      QUANTIZE_OPTIONS_TIMELINE->update_quantize_points ();
      QUANTIZE_OPTIONS_EDITOR->update_quantize_points ();
    }
  self->update_note_length = 0;
  self->update_note_type = 0;
  self->update_timesig_top = 0;
  self->update_timesig_bot = 0;

  switch (self->type)
    {
    case DigitalMeterType::DIGITAL_METER_TYPE_POSITION:
      if (self->on_drag_end)
        ((*self->on_drag_end) (self->obj));
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_BPM:
      P_TEMPO_TRACK->set_bpm (
        self->last_set_bpm, self->bpm_at_start, false, true);
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_TIMESIG:
      {
        /* no update if rolling */
        if (TRANSPORT->is_rolling ())
          {
            break;
          }

        int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
        int beat_unit = P_TEMPO_TRACK->get_beat_unit ();
        if (self->beats_per_bar_at_start != beats_per_bar)
          {
            try
              {
                UNDO_MANAGER->perform (std::make_unique<TransportAction> (
                  TransportAction::Type::BeatsPerBarChange,
                  self->beats_per_bar_at_start, beats_per_bar, true));
              }
            catch (const ZrythmException &e)
              {
                e.handle (_ ("Failed to change time signature"));
              }
          }
        else if (self->beat_unit_at_start != beat_unit)
          {
            try
              {
                UNDO_MANAGER->perform (std::make_unique<TransportAction> (
                  TransportAction::Type::BeatUnitChange,
                  self->beat_unit_at_start, beat_unit, true));
              }
            catch (const ZrythmException &e)
              {
                e.handle (_ ("Failed to change time signature"));
              }
          }
      }
      break;
    default:
      break;
    }
}

void
digital_meter_set_draw_line (DigitalMeterWidget * self, int draw_line)
{
  self->draw_line = draw_line;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
digital_meter_show_context_menu (DigitalMeterWidget * self, GMenu * menu)
{
  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, (float) gtk_widget_get_width (GTK_WIDGET (self)) / 2.f,
    0.f, menu);
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
  DigitalMeterWidget * self = Z_DIGITAL_METER_WIDGET (user_data);

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
  DigitalMeterWidget * self = Z_DIGITAL_METER_WIDGET (user_data);

  GdkEvent * event = gtk_event_controller_get_current_event (
    GTK_EVENT_CONTROLLER (scroll_controller));
  GdkScrollDirection direction = gdk_scroll_event_get_direction (event);

  int num =
    (direction == GDK_SCROLL_UP || direction == GDK_SCROLL_RIGHT) ? 1 : -1;

  update_flags (self, self->hover_x, self->hover_y);

  if (!self->scroll_started)
    {
      on_change_started (self);
      self->scroll_started = true;
    }
  self->last_scroll_time = g_get_monotonic_time ();

  ControlPort::ChangeEvent change;
  Position                 pos;
  switch (self->type)
    {
    case DigitalMeterType::DIGITAL_METER_TYPE_BPM:
      change.flag1 = PortIdentifier::Flags::Bpm;
      if (self->update_num)
        {
          change.real_val = self->last_set_bpm + (bpm_t) num;
          self->last_set_bpm = change.real_val;
          ROUTER->queue_control_port_change (change);
        }
      else if (self->update_dec)
        {
          change.real_val = self->last_set_bpm + (bpm_t) num / 100.f;
          self->last_set_bpm = change.real_val;
          ROUTER->queue_control_port_change (change);
        }

      break;

    case DigitalMeterType::DIGITAL_METER_TYPE_POSITION:
      GET_POS;
      if (display_time (self))
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
          pos.add_ms (ms);
        }
      else
        {
          if (self->update_bars)
            {
              pos.add_bars (num);
            }
          else if (self->update_beats)
            {
              pos.add_beats (num);
            }
          else if (self->update_sixteenths)
            {
              pos.add_sixteenths (num);
            }
          else if (self->update_ticks)
            {
              pos.add_ticks (num);
            }
          SET_POS;
        }
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_NOTE_LENGTH:
      if (self->update_note_length)
        {
          num += self->start_note_length;
          if (num < 0)
            *self->note_length = ENUM_INT_TO_VALUE (NoteLength, 0);
          else
            *self->note_length =
              num > ENUM_VALUE_TO_INT (NoteLength::NOTE_LENGTH_1_128)
                ? NoteLength::NOTE_LENGTH_1_128
                : ENUM_INT_TO_VALUE (NoteLength, num);
        }
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_NOTE_TYPE:
      if (self->update_note_type)
        {
          num += self->start_note_type;
          if (num < 0)
            *self->note_type = ENUM_INT_TO_VALUE (NoteType, 0);
          else
            *self->note_type =
              num > (int) NoteType::NOTE_TYPE_TRIPLET
                ? NoteType::NOTE_TYPE_TRIPLET
                : ENUM_INT_TO_VALUE (NoteType, num);
        }
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_TIMESIG:
      /* no update if rolling */
      if (TRANSPORT->is_rolling ())
        {
          break;
        }
      if (self->update_timesig_top)
        {
          num += self->beats_per_bar_at_start;
          change.flag2 = PortIdentifier::Flags2::BeatsPerBar;
          if (num < TEMPO_TRACK_MIN_BEATS_PER_BAR)
            {
              change.ival = TEMPO_TRACK_MIN_BEATS_PER_BAR;
            }
          else
            {
              change.ival =
                num > TEMPO_TRACK_MAX_BEATS_PER_BAR
                  ? TEMPO_TRACK_MAX_BEATS_PER_BAR
                  : num;
            }
          ROUTER->queue_control_port_change (change);
        }
      else if (self->update_timesig_bot)
        {
          num += (int) TempoTrack::beat_unit_to_enum (self->beat_unit_at_start);
          change.flag2 = PortIdentifier::Flags2::BeatUnit;
          if (num < 0)
            {
              change.beat_unit = BeatUnit::Two;
            }
          else
            {
              change.beat_unit =
                num > (int) BeatUnit::Sixteen
                  ? BeatUnit::Sixteen
                  : ENUM_INT_TO_VALUE (BeatUnit, num);
            }
          ROUTER->queue_control_port_change (change);
        }
      if (self->update_timesig_top || self->update_timesig_bot)
        {
          EVENTS_PUSH (EventType::ET_TIME_SIGNATURE_CHANGED, nullptr);
        }
      break;
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));

  return true;
}

static void
drag_begin (
  GtkGestureDrag *     gesture,
  gdouble              offset_x,
  gdouble              offset_y,
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
  offset_y = -offset_y;
  int      use_x = fabs (offset_x) > fabs (offset_y);
  double   diff = use_x ? offset_x - self->last_x : offset_y - self->last_y;
  int      num;
  float    dec;
  Position pos;
  switch (self->type)
    {
    case DigitalMeterType::DIGITAL_METER_TYPE_BPM:
      /*z_info ("update num ? {}", self->update_num);*/
      if (self->update_num)
        {
          num = (int) diff / 4;
          /*z_info ("updating num with {}", num);*/
          if (abs (num) > 0)
            {
              ControlPort::ChangeEvent change;
              change.flag1 = PortIdentifier::Flags::Bpm;
              change.real_val = self->last_set_bpm + (bpm_t) num;
              self->last_set_bpm = change.real_val;
              ROUTER->queue_control_port_change (change);
              self->last_y = offset_y;
              self->last_x = offset_x;
            }
        }
      else if (self->update_dec)
        {
          dec = (float) diff / 400.f;
          z_info ("{:f}", (double) dec);
          if (fabs (dec) > 0)
            {
              ControlPort::ChangeEvent change;
              change.flag1 = PortIdentifier::Flags::Bpm;
              change.real_val = self->last_set_bpm + (bpm_t) dec;
              self->last_set_bpm = change.real_val;
              ROUTER->queue_control_port_change (change);
              self->last_y = offset_y;
              self->last_x = offset_x;
            }
        }

      break;

    case DigitalMeterType::DIGITAL_METER_TYPE_POSITION:
      GET_POS;
      if (display_time (self))
        {
          if (self->update_minutes)
            {
              z_info ("UPDATE MINS");
              num = (int) diff / 4;
              if (abs (num) > 0)
                {
                  z_info ("UPDATE MINS {}", num);
                  pos.print ();
                  pos.add_minutes (num);
                  pos.print ();
                  self->last_y = offset_y;
                  self->last_x = offset_x;
                }
            }
          else if (self->update_seconds)
            {
              num = (int) diff / 4;
              if (abs (num) > 0)
                {
                  pos.add_seconds (num);
                  self->last_y = offset_y;
                  self->last_x = offset_x;
                }
            }
          else if (self->update_ms)
            {
              num = (int) diff / 4;
              if (abs (num) > 0)
                {
                  pos.add_ms (num);
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
                  pos.add_bars (num);
                  self->last_y = offset_y;
                  self->last_x = offset_x;
                }
            }
          else if (self->update_beats)
            {
              num = (int) diff / 4;
              if (abs (num) > 0)
                {
                  pos.add_beats (num);
                  self->last_y = offset_y;
                  self->last_x = offset_x;
                }
            }
          else if (self->update_sixteenths)
            {
              num = (int) diff / 4;
              if (abs (num) > 0)
                {
                  pos.add_sixteenths (num);
                  self->last_y = offset_y;
                  self->last_x = offset_x;
                }
            }
          else if (self->update_ticks)
            {
              num = (int) diff / 4;
              if (abs (num) > 0)
                {
                  pos.add_ticks (num);
                  self->last_y = offset_y;
                  self->last_x = offset_x;
                }
            }
        }
      SET_POS;
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_NOTE_LENGTH:
      if (self->update_note_length)
        {
          num = self->start_note_length + (int) offset_y / 24;
          if (num < 0)
            *self->note_length = ENUM_INT_TO_VALUE (NoteLength, 0);
          else
            *self->note_length =
              num > ENUM_VALUE_TO_INT (NoteLength::NOTE_LENGTH_1_128)
                ? NoteLength::NOTE_LENGTH_1_128
                : ENUM_INT_TO_VALUE (NoteLength, num);
        }
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_NOTE_TYPE:
      if (self->update_note_type)
        {
          num = self->start_note_type + (int) offset_y / 24;
          if (num < 0)
            *self->note_type = ENUM_INT_TO_VALUE (NoteType, 0);
          else
            *self->note_type =
              num > (int) NoteType::NOTE_TYPE_TRIPLET
                ? NoteType::NOTE_TYPE_TRIPLET
                : (NoteType) num;
        }
      break;
    case DigitalMeterType::DIGITAL_METER_TYPE_TIMESIG:
      /* no update if rolling */
      if (TRANSPORT->is_rolling ())
        {
          break;
        }
      if (self->update_timesig_top)
        {
          num = self->beats_per_bar_at_start + (int) diff / 24;
          ControlPort::ChangeEvent change;
          change.flag2 = PortIdentifier::Flags2::BeatsPerBar;
          if (num < TEMPO_TRACK_MIN_BEATS_PER_BAR)
            {
              change.ival = TEMPO_TRACK_MIN_BEATS_PER_BAR;
            }
          else
            {
              change.ival =
                num > TEMPO_TRACK_MAX_BEATS_PER_BAR
                  ? TEMPO_TRACK_MAX_BEATS_PER_BAR
                  : num;
            }
          ROUTER->queue_control_port_change (change);
        }
      else if (self->update_timesig_bot)
        {
          num =
            (int) TempoTrack::beat_unit_to_enum (self->beat_unit_at_start)
            + (int) diff / 24;
          ControlPort::ChangeEvent change;
          change.flag2 = PortIdentifier::Flags2::BeatUnit;
          if (num < 0)
            {
              change.beat_unit = BeatUnit::Two;
            }
          else
            {
              change.beat_unit =
                num > (int) BeatUnit::Sixteen ? BeatUnit::Sixteen : (BeatUnit) num;
            }
          ROUTER->queue_control_port_change (change);
        }
      if (self->update_timesig_top || self->update_timesig_bot)
        {
          EVENTS_PUSH (EventType::ET_TIME_SIGNATURE_CHANGED, nullptr);
        }
      break;
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
drag_end (
  GtkGestureDrag *     gesture,
  gdouble              offset_x,
  gdouble              offset_y,
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
  /*z_info ("{}, {}", self->height_start_pos, self->height_end_pos);*/
  update_flags (self, x, y);
}

static gboolean
tick_cb (GtkWidget * widget, GdkFrameClock * frame_clock, gpointer user_data)
{
  DigitalMeterWidget * self = Z_DIGITAL_METER_WIDGET (widget);

  gint64    cur_time = g_get_monotonic_time ();
  const int SCROLL_INTERVAL_USEC = 600 * 1000;
  if (
    self->scroll_started
    && cur_time - self->last_scroll_time > SCROLL_INTERVAL_USEC)
    {
      on_change_finished (self);
      self->scroll_started = false;
    }

  return G_SOURCE_CONTINUE;
}

/**
 * Creates a digital meter with the given type (bpm or position).
 */
DigitalMeterWidget *
digital_meter_widget_new (
  DigitalMeterType type,
  NoteLength *     note_length,
  NoteType *       note_type,
  const char *     caption)
{
  DigitalMeterWidget * self = static_cast<DigitalMeterWidget *> (
    g_object_new (DIGITAL_METER_WIDGET_TYPE, nullptr));

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
  void (*on_drag_begin) (void *),
  void (*get_val) (void *, Position *),
  void (*set_val) (void *, Position *),
  void (*on_drag_end) (void *),
  const char * caption)
{
  DigitalMeterWidget * self = static_cast<DigitalMeterWidget *> (
    g_object_new (DIGITAL_METER_WIDGET_TYPE, nullptr));

  self->obj = obj;
  self->on_drag_begin = on_drag_begin;
  self->getter = get_val;
  self->setter = set_val;
  self->on_drag_end = on_drag_end;
  self->caption = g_strdup (caption);
  self->type = DigitalMeterType::DIGITAL_METER_TYPE_POSITION;

  return self;
}

static void
dispose_digital_meter (DigitalMeterWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  G_OBJECT_CLASS (digital_meter_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
finalize_digital_meter (DigitalMeterWidget * self)
{
  if (self->caption)
    g_free (self->caption);
  if (self->caption_layout)
    g_object_unref (self->caption_layout);
  if (self->seg7_layout)
    g_object_unref (self->seg7_layout);
  if (self->normal_layout)
    g_object_unref (self->normal_layout);

  G_OBJECT_CLASS (digital_meter_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
digital_meter_widget_class_init (DigitalMeterWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = digital_meter_snapshot;
  gtk_widget_class_set_css_name (wklass, "digital-meter");
  gtk_widget_class_set_accessible_role (wklass, GTK_ACCESSIBLE_ROLE_GROUP);

  gtk_widget_class_set_layout_manager_type (wklass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize_digital_meter;
  oklass->dispose = (GObjectFinalizeFunc) dispose_digital_meter;
}

static void
digital_meter_widget_init (DigitalMeterWidget * self)
{
  gtk_widget_set_focusable (GTK_WIDGET (self), true);

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin", G_CALLBACK (drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update", G_CALLBACK (drag_update), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end", G_CALLBACK (drag_end), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag));

  GtkEventControllerMotion * motion =
    GTK_EVENT_CONTROLLER_MOTION (gtk_event_controller_motion_new ());
  g_signal_connect (G_OBJECT (motion), "motion", G_CALLBACK (on_motion), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (motion));

  GtkEventControllerScroll * scroll_controller = GTK_EVENT_CONTROLLER_SCROLL (
    gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES));
  g_signal_connect (
    G_OBJECT (scroll_controller), "scroll", G_CALLBACK (on_scroll), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (scroll_controller));

  GtkGestureClick * click_gesture = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  g_signal_connect (
    G_OBJECT (click_gesture), "pressed", G_CALLBACK (button_press_cb), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (click_gesture));

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (nullptr));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));
  gtk_widget_add_tick_callback (GTK_WIDGET (self), tick_cb, self, nullptr);
}
