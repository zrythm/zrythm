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

#include <dazzle.h>
#include <gtk/gtk.h>

#define USE_WIDE_HANDLE 1

#define TRACKLIST_WIDGET_TYPE                  (tracklist_widget_get_type ())
G_DECLARE_FINAL_TYPE (TracklistWidget,
                      tracklist_widget,
                      TRACKLIST,
                      WIDGET,
                      DzlMultiPaned)

#define MW_TRACKLIST MW_CENTER_DOCK->tracklist

typedef struct _TrackWidget TrackWidget;
typedef struct _DragDestBoxWidget DragDestBoxWidget;
typedef struct _ChordTrackWidget ChordTrackWidget;
typedef struct InstrumentTrack InstrumentTrack;
typedef struct Tracklist Tracklist;

typedef struct _TracklistWidget
{
  DzlMultiPaned                 parent_instance;
  GtkGestureDrag *              drag;
  GtkGestureMultiPress *        multipress;
  GtkGestureMultiPress *        right_mouse_mp; ///< right mouse multipress

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

void
setup_tracklist (TracklistWidget * self,
                 Tracklist * tracklist);

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


/**
 * Deletes all tracks and re-adds them as per the
 * tracklist object.
 */
void
tracklist_widget_refresh (TracklistWidget * self);

#endif
