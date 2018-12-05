/*
 * gui/widgets/chord_track.h - ChordTrack view
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

#ifndef __GUI_WIDGETS_CHORD_TRACK_H__
#define __GUI_WIDGETS_CHORD_TRACK_H__

#include "audio/channel.h"

#include <gtk/gtk.h>

#define CHORD_TRACK_WIDGET_TYPE                  (chord_track_widget_get_type ())
#define CHORD_TRACK_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHORD_TRACK_WIDGET_TYPE, ChordTrackWidget))
#define CHORD_TRACK_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), CHORD_TRACK_WIDGET, ChordTrackWidgetClass))
#define IS_CHORD_TRACK_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHORD_TRACK_WIDGET_TYPE))
#define IS_CHORD_TRACK_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), CHORD_TRACK_WIDGET_TYPE))
#define CHORD_TRACK_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), CHORD_TRACK_WIDGET_TYPE, ChordTrackWidgetClass))

typedef struct ChordTrack ChordTrack;
typedef struct ColorAreaWidget ColorAreaWidget;

/**
 * FIXME use inheritance
 */
typedef struct ChordTrackWidget
{
  GtkPaned                      parent_instance;
  GtkBox *                      track_box;
  GtkGrid *                     track_grid;
  GtkPaned *                    track_automation_paned; ///< top is the track part,
  GtkBox *                      color_box;
  ColorAreaWidget *             color;
  GtkLabel *                    track_name;
  GtkButton *                   record;
  GtkButton *                   solo;
  GtkButton *                   mute;
  GtkImage *                    icon;
  int                           selected; ///< selected or not
  ChordTrack *                  track; ///< associated chord_track
} ChordTrackWidget;

typedef struct ChordTrackWidgetClass
{
  GtkPanedClass    parent_class;
} ChordTrackWidgetClass;

/**
 * Creates a new chord_track widget from the given chord_track.
 */
ChordTrackWidget *
chord_track_widget_new (ChordTrack * chord_track);

/**
 * Updates changes in the backend to the ui
 */
void
chord_track_widget_update_all (ChordTrackWidget * self);

void
chord_track_widget_select (ChordTrackWidget * self,
                     int           select); ///< 1 = select, 0 = unselect

/**
 * Makes sure the chord_track widget and its elements have the visibility they should.
 */
void
chord_track_widget_show (ChordTrackWidget * self);

#endif

