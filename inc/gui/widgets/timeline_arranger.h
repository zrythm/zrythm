/*
 * gui/widgets/timeline_arranger.h - timeline
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#include "gui/widgets/arranger.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/piano_roll.h"
#include "audio/position.h"

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
typedef struct Chord Chord;

typedef struct _TimelineArrangerWidget
{
  ArrangerWidget           parent_instance;
  Region *                 regions[600];  ///< regions doing action upon (timeline)
  int                      num_regions;
  Region *                 start_region; ///< clicked region
  Region *                 top_region; ///< highest selected region
  Region *                 bot_region; ///< lowest selected region
  AutomationPoint *        automation_points[600];  ///< automation points doing action upon (timeline)
  AutomationPoint *        start_ap;
  int                      num_automation_points;
  AutomationCurve *        start_ac;

  /* temporary start positions, set on drag_begin, and used in drag_update
   * to move the objects accordingly */
  Position                 region_start_poses[600]; ///< region initial start positions, for moving regions
  Position                 ap_poses[600]; ///< for moving regions
  Chord *                  chords[800];
  int                      num_chords;
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

Track *
timeline_arranger_widget_get_track_at_y (double y);

AutomationTrack *
timeline_arranger_widget_get_automation_track_at_y (double y);

RegionWidget *
timeline_arranger_widget_get_hit_region (
  TimelineArrangerWidget *  self,
  double            x,
  double            y);

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
timeline_arranger_widget_update_inspector (
  TimelineArrangerWidget *self);

void
timeline_arranger_widget_select_all (
  TimelineArrangerWidget *  self,
  int                       select);

void
timeline_arranger_widget_toggle_select_region (
  TimelineArrangerWidget * self,
  Region *                 region,
  int                      append);

void
timeline_arranger_widget_toggle_select_automation_point (
  TimelineArrangerWidget *  self,
  AutomationPoint * ap,
  int               append);

/**
 * Shows context menu.
 *
 * To be called from parent on right click.
 */
void
timeline_arranger_widget_show_context_menu (
  TimelineArrangerWidget * self);

void
timeline_arranger_widget_on_drag_begin_region_hit (
  TimelineArrangerWidget * self,
  GdkModifierType          state_mask,
  double                   start_x,
  RegionWidget *           rw);

void
timeline_arranger_widget_on_drag_begin_ap_hit (
  TimelineArrangerWidget * self,
  GdkModifierType          state_mask,
  double                   start_x,
  AutomationPointWidget *  ap_widget);

void
timeline_arranger_widget_find_start_poses (
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

void
timeline_arranger_widget_find_and_select_items (
  TimelineArrangerWidget * self,
  double                   offset_x,
  double                   offset_y);

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
  int                      frames_diff);

void
timeline_arranger_widget_move_items_y (
  TimelineArrangerWidget * self,
  double                   offset_y);

void
timeline_arranger_widget_on_drag_end (
  TimelineArrangerWidget * self);

void
timeline_arranger_widget_setup (
  TimelineArrangerWidget * self);

/**
 * Readd children.
 */
void
timeline_arranger_widget_refresh_children (
  TimelineArrangerWidget * self);

#endif
