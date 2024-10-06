// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Timeline arranger API.
 */

#ifndef __GUI_WIDGETS_TIMELINE_ARRANGER_H__
#define __GUI_WIDGETS_TIMELINE_ARRANGER_H__

#include "gui/backend/gtk_widgets/arranger.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TIMELINE (MW_TIMELINE_PANEL->timeline)
#define MW_PINNED_TIMELINE (MW_TIMELINE_PANEL->pinned_timeline)

ArrangerCursor
timeline_arranger_widget_get_cursor (ArrangerWidget * self, Tool tool);

void
timeline_arranger_widget_snap_range_r (ArrangerWidget * self, Position * pos);

/**
 * Gets hit TrackLane at y.
 */
TrackLane *
timeline_arranger_widget_get_track_lane_at_y (ArrangerWidget * self, double y);

/**
 * Gets the Track at y.
 */
Track *
timeline_arranger_widget_get_track_at_y (ArrangerWidget * self, double y);

/**
 * Returns the hit AutomationTrack at y.
 */
AutomationTrack *
timeline_arranger_widget_get_at_at_y (ArrangerWidget * self, double y);

/**
 * Determines the selection time (objects/range)
 * and sets it.
 */
void
timeline_arranger_widget_set_select_type (ArrangerWidget * self, double y);

/**
 * Create a Region at the given Position in the given Track's given TrackLane.
 *
 * @param type The type of region to create.
 * @param pos The pre-snapped position.
 * @param track Track, if non-automation.
 * @param lane TrackLane, if midi/audio region.
 * @param at AutomationTrack, if automation Region.
 *
 * @throw ZrythmException on error.
 */
template <FinalRegionSubclass RegionT>
void
timeline_arranger_widget_create_region (
  ArrangerWidget * self,
  Track *          track,
  std::conditional_t<
    LaneOwnedRegionSubclass<RegionT>,
    TrackLaneImpl<RegionT> *,
    std::nullptr_t> lane,
  AutomationTrack * at,
  const Position *  pos);

/**
 * Wrapper for timeline_arranger_widget_create_chord() or
 * timeline_arranger_widget_create_scale().
 *
 * @param y the y relative to the ArrangerWidget.
 */
void
timeline_arranger_widget_create_chord_or_scale (
  ArrangerWidget * self,
  Track *          track,
  double           y,
  const Position * pos);

/**
 * Create a ScaleObject at the given Position in the given Track.
 *
 * @param pos The pre-snapped position.
 */
void
timeline_arranger_widget_create_scale (
  ArrangerWidget * self,
  Track *          track,
  const Position * pos);

/**
 * Create a Marker at the given Position in the given Track.
 *
 * @param pos The pre-snapped position.
 */
void
timeline_arranger_widget_create_marker (
  ArrangerWidget * self,
  Track *          track,
  const Position * pos);

/**
 * Scroll to the given position.
 * FIXME move to parent?
 */
void
timeline_arranger_widget_scroll_to (ArrangerWidget * self, Position * pos);

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
timeline_arranger_widget_set_cut_lines_visible (ArrangerWidget * self);

/**
 * To be called when pinning/unpinning.
 */
void
timeline_arranger_widget_remove_children (ArrangerWidget * self);

/**
 * Generate a context menu at x, y.
 *
 * @return The given updated menu or a new menu.
 */
GMenu *
timeline_arranger_widget_gen_context_menu (
  ArrangerWidget * self,
  double           x,
  double           y);

/**
 * Fade up/down.
 *
 * @param fade_in True to fade in, false to fade out.
 */
void
timeline_arranger_widget_fade_up (
  ArrangerWidget * self,
  double           offset_y,
  bool             fade_in);

/**
 * Sets up the timeline arranger as a drag dest.
 */
void
timeline_arranger_setup_drag_dest (ArrangerWidget * self);

void
timeline_arranger_on_drag_end (ArrangerWidget * self);

extern template void
timeline_arranger_widget_create_region<MidiRegion> (
  ArrangerWidget *,
  Track *,
  TrackLaneImpl<MidiRegion> *,
  AutomationTrack *,
  const Position *);
extern template void
timeline_arranger_widget_create_region<AudioRegion> (
  ArrangerWidget *,
  Track *,
  TrackLaneImpl<AudioRegion> *,
  AutomationTrack *,
  const Position *);
extern template void
timeline_arranger_widget_create_region<ChordRegion> (
  ArrangerWidget *,
  Track *,
  std::nullptr_t,
  AutomationTrack *,
  const Position *);
extern template void
timeline_arranger_widget_create_region<AutomationRegion> (
  ArrangerWidget *,
  Track *,
  std::nullptr_t,
  AutomationTrack *,
  const Position *);

/**
 * @}
 */

#endif
