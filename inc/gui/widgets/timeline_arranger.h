/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/piano_roll.h"

#include <gtk/gtk.h>

#define TIMELINE_ARRANGER_WIDGET_TYPE \
  (timeline_arranger_widget_get_type ())
G_DECLARE_FINAL_TYPE (TimelineArrangerWidget,
                      timeline_arranger_widget,
                      Z,
                      TIMELINE_ARRANGER_WIDGET,
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
typedef struct ZChord ZChord;
typedef struct _MarkerWidget MarkerWidget;

typedef struct _TimelineArrangerWidget
{
  ArrangerWidget           parent_instance;

  /** The widget first clicked on. */
  GtkWidget *          start_widget;

  /** Type of the widget first clicked on. */
  GType                start_widget_type;

  /**
   * Object first clicked is stored in start_*.
   */
  Region *                 start_region;

  /** Used to reference the region information
   * at the start of the action. */
  Region *                 start_region_clone;

  AutomationPoint *        start_ap;
  AutomationCurve *        start_ac;
  ZChord *                  start_chord;

  int                      last_timeline_obj_bars;

  /** Whether this TimelineArrangerWidget is for
   * the PinnedTracklist or not. */
  int                      is_pinned;

  /**
   * 1 if resizing range.
   */
  int                      resizing_range;

  /**
   * 1 if this is the first call to resize the range,
   * so range1 can be set.
   */
  int                      resizing_range_start;
} TimelineArrangerWidget;

/**
 * To be called from get_child_position in parent widget.
 *
 * Used to allocate the overlay children.
 */
void
timeline_arranger_widget_set_allocation (
  TimelineArrangerWidget * self,
  GtkWidget *          widget,
  GdkRectangle *       allocation);

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

AutomationTrack *
timeline_arranger_widget_get_automation_track_at_y (double y);

/**
 * Sets transient object and actual object
 * visibility based on the current action.
 */
void
timeline_arranger_widget_update_visibility (
  TimelineArrangerWidget * self);

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
ArrangerCursor
timeline_arranger_widget_get_cursor (
  TimelineArrangerWidget * self,
  UiOverlayAction action,
  Tool            tool);

/**
 * Determines the selection time (objects/range)
 * and sets it.
 */
void
timeline_arranger_widget_set_select_type (
  TimelineArrangerWidget * self,
  double                   y);

RegionWidget *
timeline_arranger_widget_get_hit_region (
  TimelineArrangerWidget *  self,
  double            x,
  double            y);

ChordWidget *
timeline_arranger_widget_get_hit_chord (
  TimelineArrangerWidget *  self,
  double                    x,
  double                    y);

MarkerWidget *
timeline_arranger_widget_get_hit_marker (
  TimelineArrangerWidget *  self,
  double                    x,
  double                    y);

AutomationPointWidget *
timeline_arranger_widget_get_hit_ap (
  TimelineArrangerWidget *  self,
  double            x,
  double            y);

AutomationCurveWidget *
timeline_arranger_widget_get_hit_curve (
  TimelineArrangerWidget * self,
  double x,
  double y);

void
timeline_arranger_widget_select_all (
  TimelineArrangerWidget *  self,
  int                       select);

/**
 * Shows context menu.
 *
 * To be called from parent on right click.
 */
void
timeline_arranger_widget_show_context_menu (
  TimelineArrangerWidget * self,
  gdouble              x,
  gdouble              y);

void
timeline_arranger_widget_on_drag_begin_region_hit (
  TimelineArrangerWidget * self,
  double                   start_x,
  RegionWidget *           rw);

void
timeline_arranger_widget_on_drag_begin_chord_hit (
  TimelineArrangerWidget * self,
  double                   start_x,
  ChordWidget *            cw);

void
timeline_arranger_widget_on_drag_begin_marker_hit (
  TimelineArrangerWidget * self,
  double                   start_x,
  MarkerWidget *           rw);

void
timeline_arranger_widget_on_drag_begin_ap_hit (
  TimelineArrangerWidget * self,
  double                   start_x,
  AutomationPointWidget *  ap_widget);

/**
 * Create an AutomationPointat the given Position
 * in the given Track's AutomationTrack.
 *
 * @param pos The pre-snapped position.
 */
void
timeline_arranger_widget_create_ap (
  TimelineArrangerWidget * self,
  AutomationTrack *  at,
  Track *            track,
  const Position *         pos,
  const double             start_y);

/**
 * Create a Region at the given Position in the
 * given Track's given TrackLane.
 *
 * @param pos The pre-snapped position.
 */
void
timeline_arranger_widget_create_region (
  TimelineArrangerWidget * self,
  Track *            track,
  TrackLane *        lane,
  const Position *         pos);

/**
 * Create a ZChord at the given Position in the
 * given Track.
 *
 * @param pos The pre-snapped position.
 */
void
timeline_arranger_widget_create_chord (
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
 * First determines the selection type (objects/
 * range), then either finds and selects items or
 * selects a range.
 *
 * @param[in] delete If this is a select-delete
 *   operation
 */
void
timeline_arranger_widget_select (
  TimelineArrangerWidget * self,
  const double             offset_x,
  const double             offset_y,
  const int                delete);

void
timeline_arranger_widget_snap_regions_l (
  TimelineArrangerWidget * self,
  Position *               pos);

void
timeline_arranger_widget_snap_regions_r (
  TimelineArrangerWidget * self,
  Position *               pos);

void
timeline_arranger_widget_move_items_x (
  TimelineArrangerWidget * self,
  long                     ticks_diff);

void
timeline_arranger_widget_move_items_y (
  TimelineArrangerWidget * self,
  double                   offset_y);

void
timeline_arranger_widget_on_drag_end (
  TimelineArrangerWidget * self);


/**
 * Sets width to ruler width and height to
 * tracklist height.
 */
void
timeline_arranger_widget_set_size (
  TimelineArrangerWidget * self);

/**
 * To be called once at init time.
 */
void
timeline_arranger_widget_setup (
  TimelineArrangerWidget * self);

/**
 * Readd children.
 */
void
timeline_arranger_widget_refresh_children (
  TimelineArrangerWidget * self);

/**
 * Refreshes visibility of children.
 */
void
timeline_arranger_widget_refresh_visibility (
  TimelineArrangerWidget * self);

/**
 * Scroll to the given position.
 */
void
timeline_arranger_widget_scroll_to (
  TimelineArrangerWidget * self,
  Position *               pos);

/**
 * @}
 */

#endif
