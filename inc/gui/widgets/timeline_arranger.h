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

#define MW_TIMELINE MW_CENTER_DOCK->timeline

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

typedef struct _TimelineArrangerWidget
{
  ArrangerWidget           parent_instance;

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

  /* These are created from the TimelineSelections so
   * the indexes match. They are set after the
   * selections are made, on drag_begin, and used in
   * drag_update to get positions relative to the
   * initial ones. */

  /* FIXME due to size limitations, create a new
   * struct that will hold this and only save its
   * pointer here */

  /** Initial start positions. */
  Position                 region_start_poses[500];

  /** Initial end positions. */
  Position                 region_end_poses[500];

  /** Initial start positions. */
  Position                 chord_start_poses[500];

  /** Initial start positions. */
  Position                 ap_poses[500];

  int                      last_timeline_obj_bars;

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

Track *
timeline_arranger_widget_get_track_at_y (double y);

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
timeline_arranger_widget_on_drag_begin_ap_hit (
  TimelineArrangerWidget * self,
  double                   start_x,
  AutomationPointWidget *  ap_widget);

/**
 * Fills in the positions that the TimelineArranger
 * remembers at the start of each drag.
 */
void
timeline_arranger_widget_set_init_poses (
  TimelineArrangerWidget * self);

void
timeline_arranger_widget_create_ap (
  TimelineArrangerWidget * self,
  AutomationTrack *        at,
  Track *                  track,
  Position *               pos,
  double                   start_y);

void
timeline_arranger_widget_create_region (
  TimelineArrangerWidget * self,
  Track *                  track,
  Position *               pos);

void
timeline_arranger_widget_create_chord (
  TimelineArrangerWidget * self,
  Track *                  track,
  Position *               pos);

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
  double                   offset_x,
  double                   offset_y,
  int                  delete);

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
timeline_arranger_widget_set_size ();

void
timeline_arranger_widget_setup ();

/**
 * Readd children.
 */
void
timeline_arranger_widget_refresh_children (
  TimelineArrangerWidget * self);

/**
 * Scroll to the given position.
 */
void
timeline_arranger_widget_scroll_to (
  TimelineArrangerWidget * self,
  Position *               pos);

#endif
