/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Timeline arranger API.
 */

#ifndef __GUI_WIDGETS_TIMELINE_ARRANGER_H__
#define __GUI_WIDGETS_TIMELINE_ARRANGER_H__

#include "audio/position.h"
#include "gui/backend/timeline_selections.h"
#include "gui/backend/tool.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/main_window.h"

#include <gtk/gtk.h>

typedef struct _ArrangerWidget ArrangerWidget;
typedef struct MidiNote        MidiNote;
typedef struct SnapGrid        SnapGrid;
typedef struct AutomationPoint AutomationPoint;
typedef struct _AutomationPointWidget
  AutomationPointWidget;
typedef struct AutomationCurve AutomationCurve;
typedef struct ChordObject     ChordObject;
typedef struct ScaleObject     ScaleObject;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TIMELINE (MW_TIMELINE_PANEL->timeline)
#define MW_PINNED_TIMELINE \
  (MW_TIMELINE_PANEL->pinned_timeline)

void
timeline_arranger_widget_snap_range_r (
  ArrangerWidget * self,
  Position *       pos);

/**
 * Gets hit TrackLane at y.
 */
TrackLane *
timeline_arranger_widget_get_track_lane_at_y (
  ArrangerWidget * self,
  double           y);

/**
 * Gets the Track at y.
 */
Track *
timeline_arranger_widget_get_track_at_y (
  ArrangerWidget * self,
  double           y);

/**
 * Returns the hit AutomationTrack at y.
 */
AutomationTrack *
timeline_arranger_widget_get_at_at_y (
  ArrangerWidget * self,
  double           y);

/**
 * Determines the selection time (objects/range)
 * and sets it.
 */
void
timeline_arranger_widget_set_select_type (
  ArrangerWidget * self,
  double           y);

/**
 * Create a ZRegion at the given Position in the
 * given Track's given TrackLane.
 *
 * @param type The type of region to create.
 * @param pos The pre-snapped position.
 * @param track Track, if non-automation.
 * @param lane TrackLane, if midi/audio region.
 * @param at AutomationTrack, if automation Region.
 */
void
timeline_arranger_widget_create_region (
  ArrangerWidget *  self,
  const RegionType  type,
  Track *           track,
  TrackLane *       lane,
  AutomationTrack * at,
  const Position *  pos);

/**
 * Wrapper for
 * timeline_arranger_widget_create_chord() or
 * timeline_arranger_widget_create_scale().
 *
 * @param y the y relative to the
 *   ArrangerWidget.
 */
void
timeline_arranger_widget_create_chord_or_scale (
  ArrangerWidget * self,
  Track *          track,
  double           y,
  const Position * pos);

/**
 * Create a ScaleObject at the given Position in the
 * given Track.
 *
 * @param pos The pre-snapped position.
 */
void
timeline_arranger_widget_create_scale (
  ArrangerWidget * self,
  Track *          track,
  const Position * pos);

/**
 * Create a Marker at the given Position in the
 * given Track.
 *
 * @param pos The pre-snapped position.
 */
void
timeline_arranger_widget_create_marker (
  ArrangerWidget * self,
  Track *          track,
  const Position * pos);

/**
 * Snaps both the transients (to show in the GUI)
 * and the actual regions.
 *
 * @param pos Absolute position in the timeline.
 * @param dry_run Don't resize notes; just check
 *   if the resize is allowed (check if invalid
 *   resizes will happen).
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
int
timeline_arranger_widget_snap_regions_l (
  ArrangerWidget * self,
  Position *       pos,
  int              dry_run);

/**
 * Snaps both the transients (to show in the GUI)
 * and the actual regions.
 *
 * @param pos Absolute position in the timeline.
 * @parram dry_run Don't resize notes; just check
 *   if the resize is allowed (check if invalid
 *   resizes will happen)
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
int
timeline_arranger_widget_snap_regions_r (
  ArrangerWidget * self,
  Position *       pos,
  int              dry_run);

/**
 * Scroll to the given position.
 * FIXME move to parent?
 */
void
timeline_arranger_widget_scroll_to (
  ArrangerWidget * self,
  Position *       pos);

/**
 * Move the selected Regions to the new Track.
 *
 * @return 1 if moved.
 */
int
timeline_arranger_move_regions_to_new_tracks (
  ArrangerWidget * self,
  const int        vis_track_diff);

/**
 * Move the selected Regions to new Lanes.
 *
 * @param diff The delta to move the
 *   Tracks.
 *
 * @return 1 if moved.
 */
int
timeline_arranger_move_regions_to_new_lanes (
  ArrangerWidget * self,
  const int        diff);

/**
 * Hides the cut dashed line from hovered regions
 * and redraws them.
 *
 * Used when alt was unpressed.
 */
void
timeline_arranger_widget_set_cut_lines_visible (
  ArrangerWidget * self);

/**
 * To be called when pinning/unpinning.
 */
void
timeline_arranger_widget_remove_children (
  ArrangerWidget * self);

/**
 * Show context menu at x, y.
 */
void
timeline_arranger_widget_show_context_menu (
  ArrangerWidget * self,
  double           x,
  double           y);

/**
 * Fade up/down.
 *
 * @param fade_in 1 for in, 0 for out.
 */
void
timeline_arranger_widget_fade_up (
  ArrangerWidget * self,
  double           offset_y,
  int              fade_in);

/**
 * Sets up the timeline arranger as a drag dest.
 */
void
timeline_arranger_setup_drag_dest (
  ArrangerWidget * self);

/**
 * @}
 */

#endif
