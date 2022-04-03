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

#ifndef __GUI_WIDGETS_MIDI_ACTIVITY_BAR_H__
#define __GUI_WIDGETS_MIDI_ACTIVITY_BAR_H__

/**
 * \file
 *
 * MIDI activity bar for tracks.
 */

#include <gtk/gtk.h>

#define MIDI_ACTIVITY_BAR_WIDGET_TYPE \
  (midi_activity_bar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MidiActivityBarWidget,
  midi_activity_bar_widget,
  Z,
  MIDI_ACTIVITY_BAR_WIDGET,
  GtkWidget)

typedef struct Track Track;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef enum MidiActivityBarType
{
  MAB_TYPE_TRACK,
  MAB_TYPE_ENGINE,
} MidiActivityBarType;

typedef enum MidiActivityBarAnimation
{
  /** Shows a bars that decreases over time. */
  MAB_ANIMATION_BAR,
  /** Shows a flash that fades out over time. */
  MAB_ANIMATION_FLASH,
} MidiActivityBarAnimation;

typedef struct _MidiActivityBarWidget
{
  GtkWidget parent_instance;

  /** Track associated with this widget. */
  Track * track;

  MidiActivityBarType type;

  MidiActivityBarAnimation animation;

  /** System time at last trigger, so we know
   * how to draw the bar (for fading down). */
  gint64 last_trigger_time;

  /** Draw border or not. */
  int draw_border;

} MidiActivityBarWidget;

/**
 * Creates a MidiActivityBarWidget for use inside
 * TrackWidget implementations.
 */
void
midi_activity_bar_widget_setup_track (
  MidiActivityBarWidget * self,
  Track *                 track);

/**
 * Sets the animation.
 */
void
midi_activity_bar_widget_set_animation (
  MidiActivityBarWidget *  self,
  MidiActivityBarAnimation animation);

/**
 * Creates a MidiActivityBarWidget for the
 * AudioEngine.
 */
void
midi_activity_bar_widget_setup_engine (
  MidiActivityBarWidget * self);

/**
 * @}
 */

#endif
