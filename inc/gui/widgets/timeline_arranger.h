/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/backend/tool.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/main_window.h"

#include <gtk/gtk.h>

#define TIMELINE_ARRANGER_WIDGET_TYPE \
  (timeline_arranger_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelineArrangerWidget,
  timeline_arranger_widget,
  Z, TIMELINE_ARRANGER_WIDGET,
  ArrangerWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TIMELINE MW_CENTER_DOCK->timeline
#define MW_PINNED_TIMELINE \
  MW_CENTER_DOCK->pinned_timeline

typedef struct _ArrangerBgWidget ArrangerBgWidget;
typedef struct MidiNote MidiNote;
typedef struct SnapGrid SnapGrid;
typedef struct AutomationPoint AutomationPoint;
typedef struct _AutomationPointWidget AutomationPointWidget;
typedef struct _AutomationCurveWidget AutomationCurveWidget;
typedef struct AutomationTrack AutomationTrack;
typedef struct AutomationCurve AutomationCurve;
typedef struct _RegionWidget RegionWidget;
typedef struct ChordObject ChordObject;
typedef struct ScaleObject ScaleObject;
typedef struct _MarkerWidget MarkerWidget;

typedef struct _TimelineArrangerWidget
{
  ArrangerWidget       parent_instance;

  /**
   * Start Region acting on. This is the
   * Region that was clicked, even though
   * there could be more selected.
   */
  Region *             start_region;

  /** Whether the start_region was selected before
   * drag_begin. */
  int                  start_region_was_selected;

  ScaleObject *        start_scale;
  Marker *             start_marker;

  int                  last_timeline_obj_bars;

  /** Whether this TimelineArrangerWidget is for
   * the PinnedTracklist or not. */
  int                  is_pinned;

  /**
   * 1 if resizing range.
   */
  int                  resizing_range;

  /**
   * 1 if this is the first call to resize the range,
   * so range1 can be set.
   */
  int                  resizing_range_start;
} TimelineArrangerWidget;

ARRANGER_W_DECLARE_FUNCS (
  Timeline, timeline);
ARRANGER_W_DECLARE_CHILD_OBJ_FUNCS (
  Timeline, timeline, Region, region);
ARRANGER_W_DECLARE_CHILD_OBJ_FUNCS (
  Timeline, timeline, ScaleObject, scale);
ARRANGER_W_DECLARE_CHILD_OBJ_FUNCS (
  Timeline, timeline, Marker, marker);

void
timeline_arranger_widget_snap_range_r (
  Position *               pos);

/**
 * Gets hit TrackLane at y.
 */
TrackLane *
timeline_arranger_widget_get_track_lane_at_y (
  TimelineArrangerWidget * self,
  double y);

/**
 * Gets the Track at y.
 */
Track *
timeline_arranger_widget_get_track_at_y (
  TimelineArrangerWidget * self,
  double y);

/**
 * Returns the hit AutomationTrack at y.
 */
AutomationTrack *
timeline_arranger_widget_get_automation_track_at_y (
  TimelineArrangerWidget * self,
  double                   y);

/**
 * Determines the selection time (objects/range)
 * and sets it.
 */
void
timeline_arranger_widget_set_select_type (
  TimelineArrangerWidget * self,
  double                   y);

/**
 * Create a Region at the given Position in the
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
  TimelineArrangerWidget * self,
  const RegionType         type,
  Track *                  track,
  TrackLane *              lane,
  AutomationTrack *        at,
  const Position *         pos);

/**
 * Wrapper for
 * timeline_arranger_widget_create_chord() or
 * timeline_arranger_widget_create_scale().
 *
 * @param y the y relative to the
 *   TimelineArrangerWidget.
 */
void
timeline_arranger_widget_create_chord_or_scale (
  TimelineArrangerWidget * self,
  Track *                  track,
  double                   y,
  const Position *         pos);

/**
 * Create a ScaleObject at the given Position in the
 * given Track.
 *
 * @param pos The pre-snapped position.
 */
void
timeline_arranger_widget_create_scale (
  TimelineArrangerWidget * self,
  Track *            track,
  const Position *         pos);

/**
 * Create a Marker at the given Position in the
 * given Track.
 *
 * @param pos The pre-snapped position.
 */
void
timeline_arranger_widget_create_marker (
  TimelineArrangerWidget * self,
  Track *            track,
  const Position *         pos);

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
timeline_arranger_widget_snap_regions_l (
  TimelineArrangerWidget * self,
  Position *               pos,
  int                      dry_run);

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
  TimelineArrangerWidget * self,
  Position *               pos,
  int                      dry_run);

/**
 * Scroll to the given position.
 * FIXME move to parent?
 */
void
timeline_arranger_widget_scroll_to (
  TimelineArrangerWidget * self,
  Position *               pos);

/**
 * Hides the cut dashed line from hovered regions
 * and redraws them.
 *
 * Used when alt was unpressed.
 */
void
timeline_arranger_widget_set_cut_lines_visible (
  TimelineArrangerWidget * self);

/**
 * @}
 */

#endif
