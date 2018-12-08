/*
 * gui/widgets/track.h - Track view
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#ifndef __GUI_WIDGETS_TRACK_H__
#define __GUI_WIDGETS_TRACK_H__

#include "audio/channel.h"

#include <gtk/gtk.h>

#define TRACK_WIDGET_TYPE \
  (track_widget_get_type ())
G_DECLARE_FINAL_TYPE (TrackWidget,
                      track_widget,
                      TRACK,
                      WIDGET,
                      GtkPaned)

typedef struct ColorAreaWidget ColorAreaWidget;
typedef struct _InstrumentTrackWidget InstrumentTrackWidget;
typedef struct _ChordTrackWidget ChordTrackWidget;
typedef struct MasterTrackWidget MasterTrackWidget;
typedef struct AudioTrackWidget AudioTrackWidget;
typedef struct BusTrackWidget BusTrackWidget;

typedef struct _TrackWidget
{
  GtkPaned                      parent_instance;
  GtkBox *                      track_box;
  GtkBox *                      color_box;
  ColorAreaWidget *             color;
  int                           selected; ///< selected or not
  int                           visible; ///< visible or not
  Track *                       track; ///< associated track

  /* depending on the track->type, only one of these is
   * non-NULL */
  InstrumentTrackWidget *       ins_tw; ///< if instrument
  ChordTrackWidget *            chord_tw; ///< if chord
  MasterTrackWidget *           master_tw;
  AudioTrackWidget *            audio_tw;
  BusTrackWidget *              bus_tw;

} TrackWidget;

/**
 * Sets up the track widget.
 *
 * Sets color, draw callback, etc.
 */
TrackWidget *
track_widget_new (Track * track);

void
track_widget_select (TrackWidget * self,
                     int           select); ///< 1 = select, 0 = unselect

/**
 * Makes sure the track widget and its elements have the visibility they should.
 */
void
track_widget_show (TrackWidget * self);

/**
 * Wrapper.
 */
void
track_widget_update_all (TrackWidget * self);

#endif

