/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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
  Z, MIDI_ACTIVITY_BAR_WIDGET,
  GtkDrawingArea)

typedef struct Track Track;

typedef struct _MidiActivityBarWidget
{
  GtkDrawingArea parent_instance;

  /** Track associated with this widget. */
  Track  *       track;

  /** System time at last trigger, so we know
   * how to draw the bar (for fading down). */
  gint64         last_trigger_time;

} MidiActivityBarWidget;

/**
 * Creates a MidiActivityBarWidget for use inside
 * TrackWidget implementations.
 */
void
midi_activity_bar_widget_setup_track (
  MidiActivityBarWidget * self,
  Track *           track);

#endif
