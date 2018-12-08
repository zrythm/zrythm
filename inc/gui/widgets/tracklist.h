/*
 * gui/instrument_timeline_view.h - The view of an instrument left of its
 *   timeline counterpart
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

#ifndef __GUI_WIDGETS_TRACKLIST_H__
#define __GUI_WIDGETS_TRACKLIST_H__

#include <gtk/gtk.h>

#define USE_WIDE_HANDLE 1

#define TRACKLIST_WIDGET_TYPE                  (tracklist_widget_get_type ())
G_DECLARE_FINAL_TYPE (TracklistWidget,
                      tracklist_widget,
                      TRACKLIST,
                      WIDGET,
                      GtkBox)

#define FOREACH_TW for (int i = 0; i < self->num_visible; i++)
#define MW_TRACKLIST MAIN_WINDOW->tracklist

typedef struct _TrackWidget TrackWidget;
typedef struct DragDestBoxWidget DragDestBoxWidget;
typedef struct _ChordTrackWidget ChordTrackWidget;
typedef struct InstrumentTrack InstrumentTrack;

typedef struct _TracklistWidget
{
  GtkBox                        parent_instance;
  GtkGestureDrag *              drag;
  GtkGestureMultiPress *        multipress;
  GtkGestureMultiPress *        right_mouse_mp; ///< right mouse multipress

  /**
   * Track widgets in order. These are the widgets that
   * are actually displayed.
   * Track widgets are created dynamically.
   */
  TrackWidget *                 visible_tw[200];
  int                           num_visible;

  /**
   * Widget for drag and dropping plugins in to create
   * new tracks.
   */
  DragDestBoxWidget *           ddbox;

  /**
   * Internal tracklist.
   */
  Tracklist *                   tracklist;
} TracklistWidget;

/**
 * Creates a new tracklist widget and sets it to main window.
 */
TracklistWidget *
tracklist_widget_new (Tracklist * tracklist);

/**
 * Adds given track to tracklist widget.
 */
void
tracklist_widget_add_track (TracklistWidget * self,
                            Track *           track,
                            int               pos);

/**
 * Removes the given track from the tracklist.
 */
void
tracklist_widget_remove_track (Track * track);

void
tracklist_widget_toggle_select_track (TracklistWidget * self,
                               Track *           track,
                               int               append); ///< append to selections

/**
 * Selects or deselects all tracks.
 */
void
tracklist_widget_toggle_select_all_tracks (TracklistWidget *self,
                                           int              select);

/**
 * Makes sure all the tracks for channels marked as visible are visible.
 */
void
tracklist_widget_show (TracklistWidget *self);

Track *
tracklist_widget_get_first_visible_track (TracklistWidget * self);

Track *
tracklist_widget_get_prev_visible_track (Track * track);

Track *
tracklist_widget_get_next_visible_track (Track * track);

Track *
tracklist_widget_get_last_visible_track (TracklistWidget * self);

#endif
