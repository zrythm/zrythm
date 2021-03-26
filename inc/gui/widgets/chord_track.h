/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_CHORD_TRACK_H__
#define __GUI_WIDGETS_CHORD_TRACK_H__

#include "audio/channel.h"
#include "gui/widgets/track.h"

#include <gtk/gtk.h>

#define CHORD_TRACK_WIDGET_TYPE \
  (chord_track_widget_get_type ())
G_DECLARE_FINAL_TYPE (ChordTrackWidget,
                      chord_track_widget,
                      Z,
                      CHORD_TRACK_WIDGET,
                      TrackWidget)

typedef struct Track ChordTrack;

typedef struct _ChordTrackWidget
{
  TrackWidget                   parent_instance;
  GtkToggleButton *             record;
  GtkToggleButton *             solo;
  GtkToggleButton *             mute;

  /**
   * Signal handler IDs.
   */
  gulong              record_toggle_handler_id;
  gulong              solo_toggled_handler_id;
  gulong              mute_toggled_handler_id;
} ChordTrackWidget;

/**
 * Creates a new chord_track widget from the given chord_track.
 */
ChordTrackWidget *
chord_track_widget_new (Track * track);

/**
 * Updates changes in the backend to the ui
 */
void
chord_track_widget_refresh (ChordTrackWidget * self);

void
chord_track_widget_refresh_buttons (
  ChordTrackWidget * self);

#endif
