// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "dsp/track.h"
#include "gui/widgets/midi_activity_bar.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/midi.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MidiActivityBarWidget, midi_activity_bar_widget, GTK_TYPE_WIDGET)

/** 250 ms */
static const double MAX_TIME = 250000.0;

static GdkRGBA other_color;
static int     other_color_set = 0;

/**
 * Draws the color picker.
 */
static void
midi_activity_bar_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  MidiActivityBarWidget * self = Z_MIDI_ACTIVITY_BAR_WIDGET (widget);

  int width = gtk_widget_get_width (widget);
  int height = gtk_widget_get_height (widget);

  if (!PROJECT || !AUDIO_ENGINE)
    {
      return;
    }

  /* draw border */
  if (self->draw_border)
    {
      GskRoundedRect  rounded_rect;
      graphene_rect_t graphene_rect =
        GRAPHENE_RECT_INIT (0.f, 0.f, (float) width, (float) height);
      gsk_rounded_rect_init_from_rect (&rounded_rect, &graphene_rect, 0);
      const float border_width = 1.f;
      GdkRGBA     border_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.2);
      float       border_widths[] = {
        border_width, border_width, border_width, border_width
      };
      GdkRGBA border_colors[] = {
        border_color, border_color, border_color, border_color
      };
      gtk_snapshot_append_border (
        snapshot, &rounded_rect, border_widths, border_colors);
    }

  /* get value */
  int trigger = 0;
  switch (self->type)
    {
    case MidiActivityBarType::MAB_TYPE_TRACK:
      trigger = self->track->trigger_midi_activity;
      break;
    case MidiActivityBarType::MAB_TYPE_ENGINE:
      trigger = AUDIO_ENGINE->trigger_midi_activity;
      break;
    }

  if (!other_color_set)
    {
      gdk_rgba_parse (&other_color, "#11FF44");
      other_color_set = 1;
    }
  if (trigger)
    {
      gtk_snapshot_append_color (
        snapshot, &other_color,
        &Z_GRAPHENE_RECT_INIT (0.f, 0.f, (float) width, (float) height));

      switch (self->type)
        {
          /* FIXME these should be UI events
           * instead of these flags */
        case MidiActivityBarType::MAB_TYPE_TRACK:
          self->track->trigger_midi_activity = 0;
          break;
        case MidiActivityBarType::MAB_TYPE_ENGINE:
          AUDIO_ENGINE->trigger_midi_activity = 0;
          break;
        }

      self->last_trigger_time = g_get_real_time ();
    }
  else
    {
      /* draw fade */
      gint64 time_diff = g_get_real_time () - self->last_trigger_time;
      if ((double) time_diff < MAX_TIME)
        {
          if (self->animation == MidiActivityBarAnimation::MAB_ANIMATION_BAR)
            {
              gtk_snapshot_append_color (
                snapshot, &other_color,
                &Z_GRAPHENE_RECT_INIT (
                  0.f, (float) height * ((float) time_diff / (float) MAX_TIME),
                  (float) width, (float) height));
            }
          else if (
            self->animation == MidiActivityBarAnimation::MAB_ANIMATION_FLASH)
            {
              other_color.alpha = 1.f - (float) time_diff / (float) MAX_TIME;
              gtk_snapshot_append_color (
                snapshot, &other_color,
                &Z_GRAPHENE_RECT_INIT (0.f, 0.f, (float) width, (float) height));
            }
        }
    }
}

static int
update_activity (
  GtkWidget *             widget,
  GdkFrameClock *         frame_clock,
  MidiActivityBarWidget * self)
{
  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

/**
 * Sets the animation.
 */
void
midi_activity_bar_widget_set_animation (
  MidiActivityBarWidget *  self,
  MidiActivityBarAnimation animation)
{
  self->animation = animation;
}

/**
 * Creates a MidiActivityBarWidget for use inside
 * TrackWidget implementations.
 */
void
midi_activity_bar_widget_setup_track (MidiActivityBarWidget * self, Track * track)
{
  self->track = track;
  self->type = MidiActivityBarType::MAB_TYPE_TRACK;
  self->draw_border = false;

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) update_activity, self, NULL);
}

/**
 * Creates a MidiActivityBarWidget for the
 * AudioEngine.
 */
void
midi_activity_bar_widget_setup_engine (MidiActivityBarWidget * self)
{
  self->type = MidiActivityBarType::MAB_TYPE_ENGINE;
  self->draw_border = true;

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) update_activity, self, NULL);
}

static void
midi_activity_bar_widget_init (MidiActivityBarWidget * self)
{
  gtk_widget_set_size_request (GTK_WIDGET (self), 4, -1);
}

static void
midi_activity_bar_widget_class_init (MidiActivityBarWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  gtk_widget_class_set_css_name (wklass, "midi-activity-bar");
  wklass->snapshot = midi_activity_bar_snapshot;
}
