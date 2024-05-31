/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
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

#include "gtk_wrapper.h"

#define PINNED_TRACKLIST_WIDGET_TYPE (pinned_tracklist_widget_get_type ())
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

#define MW_PINNED_TRACKLIST MW_TIMELINE_PANEL->pinned_tracklist

struct Tracklist;
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
pinned_tracklist_widget_hard_refresh (PinnedTracklistWidget * self);

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
