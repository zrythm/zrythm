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

/**
 * \file
 *
 * The ruler tracklist contains special tracks that
 * are shown above the normal tracklist (Chord
 * tracks, Marker tracks, etc.).
 */

#ifndef __GUI_WIDGETS_PINNED_TRACKLIST_H__
#define __GUI_WIDGETS_PINNED_TRACKLIST_H__

#include "dsp/region.h"
#include "gui/widgets/region.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#define PINNED_TRACKLIST_WIDGET_TYPE \
  (pinned_tracklist_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PinnedTracklistWidget,
  pinned_tracklist_widget,
  Z,
  PINNED_TRACKLIST_WIDGET,
  GtkBox);

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_PINNED_TRACKLIST \
  MW_TIMELINE_PANEL->pinned_tracklist

typedef struct Tracklist    Tracklist;
typedef struct _TrackWidget TrackWidget;

/**
 * The PinnedTracklistWidget contains special tracks
 * (chord, marker, etc.) as thin boxes above the
 * normal tracklist.
 *
 * The contents of each track will be shown in the
 * PinnedTracklistArrangerWidget.
 */
typedef struct _PinnedTracklistWidget
{
  GtkBox parent_instance;

  /** The backend. */
  Tracklist * tracklist;

} PinnedTracklistWidget;

/**
 * Gets TrackWidget hit at the given coordinates.
 */
TrackWidget *
pinned_tracklist_widget_get_hit_track (
  PinnedTracklistWidget * self,
  double                  x,
  double                  y);

/**
 * Removes and readds the tracks.
 */
void
pinned_tracklist_widget_hard_refresh (
  PinnedTracklistWidget * self);

/**
 * Sets up the PinnedTracklistWidget.
 */
void
pinned_tracklist_widget_setup (
  PinnedTracklistWidget * self,
  Tracklist *             tracklist);

/**
 * @}
 */

#endif
