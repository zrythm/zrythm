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

#ifndef __GUI_WIDGETS_TRACKLIST_H__
#define __GUI_WIDGETS_TRACKLIST_H__

#include <gtk/gtk.h>

#include "gui/widgets/dzl/dzl-multi-paned.h"

#define USE_WIDE_HANDLE 1

#define TRACKLIST_WIDGET_TYPE \
  (tracklist_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TracklistWidget,
  tracklist_widget,
  Z, TRACKLIST_WIDGET,
  DzlMultiPaned)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TRACKLIST MW_CENTER_DOCK->tracklist

typedef struct _TrackWidget TrackWidget;
typedef struct _DragDestBoxWidget DragDestBoxWidget;
typedef struct _ChordTrackWidget ChordTrackWidget;
typedef struct Track InstrumentTrack;
typedef struct Tracklist Tracklist;

/**
 * The TracklistWidget is a DzlMultiPaned holding
 * all the Track's in the Project.
 */
typedef struct _TracklistWidget
{
  DzlMultiPaned                 parent_instance;

  /**
   * Widget for drag and dropping plugins in to
   * create new tracks.
   */
  DragDestBoxWidget *           ddbox;

  /**
   * Internal tracklist.
   */
  Tracklist *                   tracklist;
} TracklistWidget;

/**
 * Sets up the TracklistWidget.
 */
void
tracklist_widget_setup (
  TracklistWidget * self,
  Tracklist * tracklist);

/**
 * Selects the track, with the option to either
 * add the track to the current selection or to
 * select it exclusively.
 */
void
tracklist_widget_select_track (
  TracklistWidget * self,
  Track *           track,
  int               select,
  int               append);

/**
 * Selects or deselects all tracks.
 */
void
tracklist_widget_select_all_tracks (
  TracklistWidget *self,
  int              select);

/**
 * Makes sure all the tracks for channels marked as
 * visible are visible.
 */
void
tracklist_widget_soft_refresh (
  TracklistWidget *self);


/**
 * Gets hit TrackWidget and the given coordinates.
 */
TrackWidget *
tracklist_widget_get_hit_track (
  TracklistWidget *  self,
  double            x,
  double            y);

/**
 * Deletes all tracks and re-adds them.
 */
void
tracklist_widget_hard_refresh (
  TracklistWidget * self);

/**
 * @}
 */

#endif
