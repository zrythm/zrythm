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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/engine.h"
#include "audio/midi.h"
#include "audio/track.h"
#include "gui/widgets/midi_activity_bar.h"
#include "gui/widgets/track.h"
#include "gui/widgets/track_top_grid.h"
#include "project.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MidiActivityBarWidget,
               midi_activity_bar_widget,
               GTK_TYPE_DRAWING_AREA)

/** 250 ms */
static const double MAX_TIME = 250000.0;

static GdkRGBA other_color;
static int other_color_set = 0;

/**
 * Draws the color picker.
 */
static bool
midi_activity_bar_draw_cb (
  GtkWidget *       widget,
  cairo_t *         cr,
  MidiActivityBarWidget * self)
{
  GdkRGBA color;

  if (!PROJECT || !AUDIO_ENGINE)
    {
      return false;
    }

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  /* draw border */
  if (self->draw_border)
    {
      color.red = 1.0;
      color.green = 1.0;
      color.blue = 1.0;
      color.alpha = 0.2;
      gdk_cairo_set_source_rgba (cr, &color);
      cairo_rectangle (cr, 0, 0, width, height);
      cairo_stroke (cr);
    }

  /* get value */
  int trigger = 0;
  switch (self->type)
    {
    case MAB_TYPE_TRACK:
      trigger = self->track->trigger_midi_activity;
      break;
    case MAB_TYPE_ENGINE:
      trigger = AUDIO_ENGINE->trigger_midi_activity;
      break;
    }

  if (!other_color_set)
    {
      gdk_rgba_parse (&other_color, "#11FF44");
      other_color_set = 1;
    }
  gdk_cairo_set_source_rgba (cr, &other_color);
  if (trigger)
    {
      cairo_rectangle (cr, 0, 0, width, height);
      cairo_fill (cr);

      switch (self->type)
        {
        case MAB_TYPE_TRACK:
          self->track->trigger_midi_activity = 0;
          break;
        case MAB_TYPE_ENGINE:
          AUDIO_ENGINE->trigger_midi_activity = 0;
          break;
        }

      self->last_trigger_time =
        g_get_real_time ();
    }
  else
    {
      /* draw fade */
      gint64 time_diff =
        g_get_real_time () -
        self->last_trigger_time;
      if ((double) time_diff < MAX_TIME)
        {
          if (self->animation == MAB_ANIMATION_BAR)
            {
              cairo_rectangle (
                cr, 0,
                (double) height *
                ((double) time_diff / MAX_TIME),
                width, height);
            }
          else if (self->animation ==
                     MAB_ANIMATION_FLASH)
            {
              other_color.alpha =
                1.0 -
                (double) time_diff / MAX_TIME;
              gdk_cairo_set_source_rgba (
                cr, &other_color);
              cairo_rectangle (
                cr, 0, 0, width, height);
            }
          cairo_fill (cr);
        }
    }

  return FALSE;
}

static int
update_activity (
  GtkWidget * widget,
  GdkFrameClock * frame_clock,
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
  MidiActivityBarWidget * self,
  MidiActivityBarAnimation animation)
{
  self->animation = animation;
}

/**
 * Creates a MidiActivityBarWidget for use inside
 * TrackWidget implementations.
 */
void
midi_activity_bar_widget_setup_track (
  MidiActivityBarWidget * self,
  Track *           track)
{
  self->track = track;
  self->type = MAB_TYPE_TRACK;
  self->draw_border = 0;

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (midi_activity_bar_draw_cb), self);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self),
    (GtkTickCallback) update_activity,
    self, NULL);
}

/**
 * Creates a MidiActivityBarWidget for the
 * AudioEngine.
 */
void
midi_activity_bar_widget_setup_engine (
  MidiActivityBarWidget * self)
{
  self->type = MAB_TYPE_ENGINE;
  self->draw_border = 1;

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (midi_activity_bar_draw_cb), self);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self),
    (GtkTickCallback) update_activity,
    self, NULL);
}

static void
midi_activity_bar_widget_init (
  MidiActivityBarWidget * self)
{
  gtk_widget_set_size_request (
    GTK_WIDGET (self), 4, -1);
}

static void
midi_activity_bar_widget_class_init (
  MidiActivityBarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "midi-activity-bar");
}

